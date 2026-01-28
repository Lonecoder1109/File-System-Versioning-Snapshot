#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new snapshot
uint32_t fs_create_snapshot(FileSystem *fs, const char *name, const char *description) {
    if (fs->snapshot_count >= MAX_SNAPSHOTS) {
        return (uint32_t)-1;
    }
    
    clock_t start = clock();
    
    Snapshot *snapshot = &fs->snapshots[fs->snapshot_count];
    snapshot->snapshot_id = fs->snapshot_count + 1;
    strncpy(snapshot->name, name, MAX_FILENAME - 1);
    strncpy(snapshot->description, description, 511);
    snapshot->created_at = time(NULL);
    snapshot->total_size = 0;
    snapshot->inode_count = fs->used_inodes;
    snapshot->inodes = (uint32_t *)malloc(fs->used_inodes * sizeof(uint32_t));
    snapshot->parent_snapshot = 0;
    snapshot->child_snapshots = NULL;
    snapshot->child_count = 0;
    snapshot->group_name[0] = '\0';
    snapshot->granularity = fs->default_granularity;
    snapshot->operations_since_last = 0;
    snapshot->importance_score = 1.0;
    snapshot->tag_count = 0;
    snapshot->is_trimmed = false;
    snapshot->ref_count = 1;
    
    // Capture current state of all inodes
    uint32_t idx = 0;
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].inode_id != 0) {
            snapshot->inodes[idx++] = fs->inodes[i].inode_id;
            snapshot->total_size += fs->inodes[i].size;

            // ⭐ NEW: Store which version this inode is at in the map
            uint32_t inode_id = fs->inodes[i].inode_id;
            uint32_t current_ver = fs->inodes[i].current_version;
            fs->snapshot_version_map[snapshot->snapshot_id - 1][inode_id] = current_ver;
            
            /* Increment ref count on all blocks (CoW support)
            for (uint32_t b = 0; b < fs->inodes[i].block_count; b++) {
                fs->blocks[fs->inodes[i].blocks[b]].ref_count++;***
            }*/
        }
    }
    
    fs->snapshot_count++;
    fs->metrics.total_snapshots++;
    fs->is_dirty = true;
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    fs->metrics.avg_snapshot_time = (fs->metrics.avg_snapshot_time * (fs->metrics.total_snapshots - 1) + time_taken) / fs->metrics.total_snapshots;
    
    return snapshot->snapshot_id;
}

// Delete a snapshot
bool fs_delete_snapshot(FileSystem *fs, uint32_t snapshot_id) {
    if (snapshot_id == 0 || snapshot_id > fs->snapshot_count) return false;
    
    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0) return false;
    
    snapshot->ref_count--;
    
    if (snapshot->ref_count == 0) {
        // Decrease ref count on all blocks
        for (uint32_t i = 0; i < snapshot->inode_count; i++) {
            Inode *inode = fs_get_inode(fs, snapshot->inodes[i]);
            if (inode) {
                for (uint32_t b = 0; b < inode->block_count; b++) {
                    fs_free_block(fs, inode->blocks[b]);
                }
            }
        }
        
        if (snapshot->inodes) {
            free(snapshot->inodes);
        }
        if (snapshot->child_snapshots) {
            free(snapshot->child_snapshots);
        }
        
        memset(snapshot, 0, sizeof(Snapshot));
        fs->is_dirty = true;
    }
    
    return true;
}

// Rollback to a snapshot
bool fs_rollback_snapshot(FileSystem *fs, uint32_t snapshot_id) {
    if (snapshot_id == 0 || snapshot_id > fs->snapshot_count) return false;
    
    clock_t start = clock();
    
    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0) return false;
    
    printf("Rolling back to snapshot: %s (ID: %u)\n", snapshot->name, snapshot_id);
    
    // For each file in the snapshot, find and rollback to the snapshot version
    for (uint32_t i = 0; i < snapshot->inode_count; i++) {
    Inode *inode = fs_get_inode(fs, snapshot->inodes[i]);
    if (!inode) continue;
    
    // ⭐ NEW: Get the version from the map
    uint32_t target_version = fs->snapshot_version_map[snapshot_id - 1][inode->inode_id];
    
    if (target_version > 0 && target_version <= inode->version_count) {
        printf("  Rolling back file %s to version %u\n", 
               inode->filename, target_version);
        fs_rollback_version(fs, inode->inode_id, target_version);
    }
}
    
    fs->metrics.total_rollbacks++;
    fs->is_dirty = true;
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    fs->metrics.avg_rollback_time = (fs->metrics.avg_rollback_time * (fs->metrics.total_rollbacks - 1) + time_taken) / fs->metrics.total_rollbacks;
    
    return true;
}

// Trim a snapshot (remove redundant data)
bool fs_trim_snapshot(FileSystem *fs, uint32_t snapshot_id) {
    if (snapshot_id == 0 || snapshot_id > fs->snapshot_count) return false;
    
    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0 || snapshot->is_trimmed) return false;
    
    // Mark as trimmed
    snapshot->is_trimmed = true;
    
    // In a real implementation, you would:
    // 1. Identify blocks that are duplicated in other snapshots
    // 2. Reduce ref counts on those blocks
    // 3. Update the snapshot metadata
    
    printf("Trimmed snapshot: %s (ID: %u)\n", snapshot->name, snapshot_id);
    
    fs->is_dirty = true;
    return true;
}

// Add tag to snapshot
bool fs_add_snapshot_tag(FileSystem *fs, uint32_t snapshot_id, const char *tag, const char *description) {
    if (!fs || snapshot_id == 0 || snapshot_id > fs->snapshot_count) return false;

    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0) return false;

    if (snapshot->tag_count >= MAX_TAGS_PER_VERSION) return false;

    // Copy tag safely
    strncpy(snapshot->tags[snapshot->tag_count].tag, tag, MAX_TAG_LENGTH - 1);
    snapshot->tags[snapshot->tag_count].tag[MAX_TAG_LENGTH - 1] = '\0';

    // Copy description safely
    strncpy(snapshot->tags[snapshot->tag_count].description, description, 255);
    snapshot->tags[snapshot->tag_count].description[255] = '\0';

    snapshot->tags[snapshot->tag_count].created_at = time(NULL);
    snapshot->tag_count++;
    fs->is_dirty = true;

    printf("[DEBUG] Added tag '%s' to snapshot ID %u (Total tags: %u)\n",
           tag, snapshot_id, snapshot->tag_count);

    return true;
}

// Find snapshots by tag
Snapshot* fs_find_snapshots_by_tag(FileSystem *fs, const char *tag, uint32_t *count) {
    *count = 0;
    Snapshot *results = (Snapshot *)malloc(fs->snapshot_count * sizeof(Snapshot));
    
    for (uint32_t i = 0; i < fs->snapshot_count; i++) {
        Snapshot *snapshot = &fs->snapshots[i];
        if (snapshot->snapshot_id == 0) continue;
        
        for (uint32_t t = 0; t < snapshot->tag_count; t++) {
            if (strcmp(snapshot->tags[t].tag, tag) == 0) {
                results[*count] = *snapshot;
                (*count)++;
                break;
            }
        }
    }
    
    return results;
}

// Create snapshot group
bool fs_create_snapshot_group(FileSystem *fs, const char *group_name, uint32_t *snapshot_ids, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (snapshot_ids[i] == 0 || snapshot_ids[i] > fs->snapshot_count) continue;
        
        Snapshot *snapshot = &fs->snapshots[snapshot_ids[i] - 1];
        if (snapshot->snapshot_id == 0) continue;
        
        strncpy(snapshot->group_name, group_name, MAX_FILENAME - 1);
    }
    
    fs->is_dirty = true;
    return true;
}

// Set snapshot parent (for hierarchical grouping)
bool fs_set_snapshot_parent(FileSystem *fs, uint32_t snapshot_id, uint32_t parent_id) {
    if (snapshot_id == 0 || snapshot_id > fs->snapshot_count) return false;
    if (parent_id > fs->snapshot_count) return false;
    
    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0) return false;
    
    snapshot->parent_snapshot = parent_id;
    
    // Add to parent's children
    if (parent_id > 0) {
        Snapshot *parent = &fs->snapshots[parent_id - 1];
        if (parent->snapshot_id != 0) {
            parent->child_snapshots = (uint32_t *)realloc(parent->child_snapshots, 
                                                          (parent->child_count + 1) * sizeof(uint32_t));
            parent->child_snapshots[parent->child_count] = snapshot_id;
            parent->child_count++;
        }
    }
    
    fs->is_dirty = true;
    return true;
}

// Get snapshot hierarchy
Snapshot** fs_get_snapshot_hierarchy(FileSystem *fs, uint32_t root_snapshot_id, uint32_t *count) {
    *count = 0;
    
    if (root_snapshot_id == 0 || root_snapshot_id > fs->snapshot_count) return NULL;
    
    Snapshot *root = &fs->snapshots[root_snapshot_id - 1];
    if (root->snapshot_id == 0) return NULL;
    
    // Allocate result array (max possible size)
    Snapshot **results = (Snapshot **)malloc(fs->snapshot_count * sizeof(Snapshot *));
    
    // Add root
    results[*count] = root;
    (*count)++;
    
    // Recursively add children (simplified - doesn't handle deep recursion)
    for (uint32_t i = 0; i < root->child_count; i++) {
        uint32_t child_id = root->child_snapshots[i];
        if (child_id > 0 && child_id <= fs->snapshot_count) {
            Snapshot *child = &fs->snapshots[child_id - 1];
            if (child->snapshot_id != 0) {
                results[*count] = child;
                (*count)++;
            }
        }
    }
    
    return results;
}

// Update snapshot importance score (for adaptive granularity)
void fs_update_snapshot_importance(FileSystem *fs, uint32_t snapshot_id) {
    if (snapshot_id == 0 || snapshot_id > fs->snapshot_count) return;
    
    Snapshot *snapshot = &fs->snapshots[snapshot_id - 1];
    if (snapshot->snapshot_id == 0) return;
    
    // Calculate importance based on:
    // 1. Age (older = less important)
    // 2. Size (larger = more important)
    // 3. Number of tags (more tags = more important)
    // 4. Reference count (more refs = more important)
    
    time_t now = time(NULL);
    double age_days = difftime(now, snapshot->created_at) / (24 * 3600);
    
    double age_factor = 1.0 / (1.0 + age_days / 30.0);  // Decay over 30 days
    double size_factor = (double)snapshot->total_size / (1024 * 1024);  // MB
    double tag_factor = 1.0 + (double)snapshot->tag_count * 0.5;
    double ref_factor = (double)snapshot->ref_count;
    
    snapshot->importance_score = age_factor * size_factor * tag_factor * ref_factor;
}

// Check if should create snapshot (adaptive granularity)
bool fs_should_create_snapshot(FileSystem *fs) {
    if (!fs->auto_snapshot_enabled) return false;
    
    // Get the last snapshot
    if (fs->snapshot_count == 0) return true;
    
    Snapshot *last_snapshot = &fs->snapshots[fs->snapshot_count - 1];
    last_snapshot->operations_since_last++;
    
    uint32_t threshold = fs->auto_snapshot_threshold;
    
    // Adjust threshold based on granularity
    switch (last_snapshot->granularity) {
        case GRANULARITY_FINE:
            threshold /= 2;
            break;
        case GRANULARITY_COARSE:
            threshold *= 2;
            break;
        default:
            break;
    }
    
    return last_snapshot->operations_since_last >= threshold;
}

// Adjust snapshot granularity
void fs_adjust_granularity(FileSystem *fs, SnapshotGranularity granularity) {
    fs->default_granularity = granularity;
    
    // Update threshold based on granularity
    switch (granularity) {
        case GRANULARITY_FINE:
            fs->auto_snapshot_threshold = 50;
            break;
        case GRANULARITY_MEDIUM:
            fs->auto_snapshot_threshold = 100;
            break;
        case GRANULARITY_COARSE:
            fs->auto_snapshot_threshold = 200;
            break;
    }
}

// List all snapshots
void fs_list_snapshots(FileSystem *fs) {
    printf("\n=== Snapshot List ===\n");
    for (uint32_t i = 0; i < fs->snapshot_count; i++) {
        Snapshot *snapshot = &fs->snapshots[i];
        if (snapshot->snapshot_id != 0) {
            printf("ID: %u | Name: %s | Size: %lu bytes | Inodes: %u | Tags: %u | Group: %s\n",
                   snapshot->snapshot_id, snapshot->name, snapshot->total_size,
                   snapshot->inode_count, snapshot->tag_count, 
                   snapshot->group_name[0] ? snapshot->group_name : "None");
            
            if (snapshot->parent_snapshot > 0) {
                printf("  Parent: %u | Children: %u\n", 
                       snapshot->parent_snapshot, snapshot->child_count);
            }
        }
    }
}