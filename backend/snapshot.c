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
    snapshot->total_size = 0;
    snapshot->inode_count = fs->used_inodes;
    snapshot->inodes = (uint32_t *)malloc(fs->used_inodes * sizeof(uint32_t));
    snapshot->ref_count = 1;
    
    // Capture current state of all inodes
    uint32_t idx = 0;
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].inode_id != 0) {
            snapshot->inodes[idx++] = fs->inodes[i].inode_id;
            snapshot->total_size += fs->inodes[i].size;

            // Store which version this inode is at in the map
            uint32_t inode_id = fs->inodes[i].inode_id;
            uint32_t current_ver = fs->inodes[i].current_version;
            fs->snapshot_version_map[snapshot->snapshot_id - 1][inode_id] = current_ver;
            
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
    
    // Get the version from the map
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

