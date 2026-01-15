#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// SHA-256 simple hash implementation for deduplication
void fs_compute_hash(const void *data, size_t size, Hash *hash) {
    // Simple hash function (in production, use proper SHA-256)
    uint32_t h = 0x12345678;
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < size; i++) {
        h = ((h << 5) + h) + bytes[i];
    }
    
    memset(hash->hash, 0, HASH_SIZE);
    memcpy(hash->hash, &h, sizeof(h));
    
    // Add more entropy
    for (int i = 4; i < HASH_SIZE; i++) {
        hash->hash[i] = (uint8_t)((h >> (i % 4)) ^ bytes[i % size]);
    }
}

// Compare two hashes
bool hash_equals(const Hash *h1, const Hash *h2) {
    return memcmp(h1->hash, h2->hash, HASH_SIZE) == 0;
}

// Create a new filesystem
FileSystem* fs_create(const char *disk_file, uint32_t total_blocks, uint32_t total_inodes) {
    FileSystem *fs = (FileSystem *)calloc(1, sizeof(FileSystem));
    if (!fs) return NULL;
    
    // Initialize blocks
    fs->total_blocks = total_blocks;
    fs->used_blocks = 0;
    fs->blocks = (BlockMetadata *)calloc(total_blocks, sizeof(BlockMetadata));
    if (!fs->blocks) {
        free(fs);
        return NULL;
    }
    fs->block_data = (uint8_t **)calloc(total_blocks, sizeof(uint8_t *));
    if (!fs->block_data) {
        free(fs->blocks);
        free(fs);
        return NULL;
    }
    
    for (uint32_t i = 0; i < total_blocks; i++) {
        fs->blocks[i].block_id = i;
        fs->blocks[i].type = BLOCK_FREE;
        fs->blocks[i].ref_count = 0;
        fs->block_data[i] = NULL;
    }
    
    // Initialize inodes
    fs->total_inodes = total_inodes;
    fs->used_inodes = 0;
    fs->inodes = (Inode *)calloc(total_inodes, sizeof(Inode));
    if (!fs->inodes) {
        free(fs->block_data);
        free(fs->blocks);
        free(fs);
        return NULL;
    }
    
    // Initialize snapshots
    fs->snapshots = (Snapshot *)calloc(MAX_SNAPSHOTS, sizeof(Snapshot));
    if (!fs->snapshots) {
        free(fs->inodes);
        free(fs->block_data);
        free(fs->blocks);
        free(fs);
        return NULL;
    }
    fs->snapshot_count = 0;
    
    // Initialize journal
    fs->journal = (JournalEntry *)calloc(JOURNAL_SIZE, sizeof(JournalEntry));
    if (!fs->journal) {
        free(fs->snapshots);
        free(fs->inodes);
        free(fs->block_data);
        free(fs->blocks);
        free(fs);
        return NULL;
    }
    fs->journal_head = 0;
    fs->journal_tail = 0;
    fs->next_transaction_id = 1;
    
    // Initialize deduplication table
    fs->dedup_table = (DedupEntry *)calloc(MAX_BLOCKS, sizeof(DedupEntry));
    if (!fs->dedup_table) {
        free(fs->journal);
        free(fs->snapshots);
        free(fs->inodes);
        free(fs->block_data);
        free(fs->blocks);
        free(fs);
        return NULL;
    }
    fs->dedup_count = 0;
    
    // Configuration
    fs->default_strategy = STRATEGY_COW;
    fs->default_granularity = GRANULARITY_MEDIUM;
    fs->auto_snapshot_enabled = true;
    fs->auto_snapshot_threshold = 100;
    
    // Persistence
    strncpy(fs->disk_file, disk_file, MAX_PATH - 1);
    fs->is_dirty = true;
    
    // Initialize metrics
    memset(&fs->metrics, 0, sizeof(PerformanceMetrics));
    
    return fs;
}

// Destroy filesystem
void fs_destroy(FileSystem *fs) {
    if (!fs) return;
    
    // Free block data
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->block_data[i]) {
            free(fs->block_data[i]);
        }
    }
    free(fs->block_data);
    free(fs->blocks);
    
    // Free inode data
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].blocks) {
            free(fs->inodes[i].blocks);
        }
        if (fs->inodes[i].versions) {
            for (uint32_t v = 0; v < fs->inodes[i].version_count; v++) {
                if (fs->inodes[i].versions[v].blocks) {
                    free(fs->inodes[i].versions[v].blocks);
                }
            }
            free(fs->inodes[i].versions);
        }
    }
    free(fs->inodes);
    
    // Free snapshots
    for (uint32_t i = 0; i < fs->snapshot_count; i++) {
        if (fs->snapshots[i].inodes) {
            free(fs->snapshots[i].inodes);
        }
        if (fs->snapshots[i].child_snapshots) {
            free(fs->snapshots[i].child_snapshots);
        }
    }
    free(fs->snapshots);
    
    free(fs->journal);
    free(fs->dedup_table);
    free(fs);
}

// Format filesystem (reset to clean state)
void fs_format(FileSystem *fs) {
    if (!fs) return;
    
    // Reset blocks
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        fs->blocks[i].block_id = i;
        fs->blocks[i].type = BLOCK_FREE;
        fs->blocks[i].ref_count = 0;
        fs->blocks[i].is_cow = false;
        fs->blocks[i].is_deduplicated = false;
        
        if (fs->block_data[i]) {
            memset(fs->block_data[i], 0, BLOCK_SIZE);
        }
    }
    fs->used_blocks = 0;
    
    // Reset inodes
    memset(fs->inodes, 0, fs->total_inodes * sizeof(Inode));
    fs->used_inodes = 0;
    
    // Reset snapshots
    memset(fs->snapshots, 0, MAX_SNAPSHOTS * sizeof(Snapshot));
    fs->snapshot_count = 0;
    
    // Reset journal
    fs->journal_head = 0;
    fs->journal_tail = 0;
    fs->next_transaction_id = 1;
    
    // Reset metrics
    memset(&fs->metrics, 0, sizeof(PerformanceMetrics));
    
    // Reset dedup table
    memset(fs->dedup_table, 0, MAX_BLOCKS * sizeof(DedupEntry));
    fs->dedup_count = 0;
    
    fs->is_dirty = true;
    printf("Filesystem formatted.\n");
}

// Allocate a block
uint32_t fs_allocate_block(FileSystem *fs, BlockType type) {
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].type == BLOCK_FREE) {
            fs->blocks[i].type = type;
            fs->blocks[i].ref_count = 1;
            fs->blocks[i].created_at = time(NULL);
            fs->blocks[i].modified_at = time(NULL);
            fs->blocks[i].is_cow = false;
            fs->blocks[i].is_deduplicated = false;
            
            // Allocate actual data
            if (!fs->block_data[i]) {
                fs->block_data[i] = (uint8_t *)calloc(BLOCK_SIZE, 1);
            }
            
            fs->used_blocks++;
            fs->metrics.blocks_allocated++;
            fs->is_dirty = true;
            
            return i;
        }
    }
    return (uint32_t)-1; // No free blocks
}

// Free a block
void fs_free_block(FileSystem *fs, uint32_t block_id) {
    if (block_id >= fs->total_blocks) return;
    
    BlockMetadata *block = &fs->blocks[block_id];
    
    if (block->ref_count > 0) {
        block->ref_count--;
    }
    
    if (block->ref_count == 0) {
        block->type = BLOCK_FREE;
        if (fs->block_data[block_id]) {
            memset(fs->block_data[block_id], 0, BLOCK_SIZE);
        }
        fs->used_blocks--;
        fs->metrics.blocks_freed++;
        fs->is_dirty = true;
    }
}

// Read block
bool fs_read_block(FileSystem *fs, uint32_t block_id, void *buffer) {
    if (block_id >= fs->total_blocks || !buffer) return false;
    if (fs->blocks[block_id].type == BLOCK_FREE) return false;
    
    memcpy(buffer, fs->block_data[block_id], BLOCK_SIZE);
    fs->metrics.total_reads++;
    
    return true;
}

// Write block
bool fs_write_block(FileSystem *fs, uint32_t block_id, const void *buffer) {
    if (block_id >= fs->total_blocks || !buffer) return false;
    if (fs->blocks[block_id].type == BLOCK_FREE) return false;
    
    clock_t start = clock();
    
    memcpy(fs->block_data[block_id], buffer, BLOCK_SIZE);
    fs->blocks[block_id].modified_at = time(NULL);
    
    // Compute hash for deduplication
    fs_compute_hash(buffer, BLOCK_SIZE, &fs->blocks[block_id].content_hash);
    
    fs->metrics.total_writes++;
    fs->is_dirty = true;
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    fs->metrics.avg_write_time = (fs->metrics.avg_write_time * (fs->metrics.total_writes - 1) + time_taken) / fs->metrics.total_writes;
    
    return true;
}

// Copy-on-Write: Create a copy of a block
uint32_t fs_cow_block(FileSystem *fs, uint32_t original_block) {
    if (original_block >= fs->total_blocks) return (uint32_t)-1;
    
    uint32_t new_block = fs_allocate_block(fs, fs->blocks[original_block].type);
    if (new_block == (uint32_t)-1) return (uint32_t)-1;
    
    // Copy data
    memcpy(fs->block_data[new_block], fs->block_data[original_block], BLOCK_SIZE);
    
    // Mark as CoW
    fs->blocks[new_block].is_cow = true;
    fs->blocks[new_block].original_block = original_block;
    fs->blocks[new_block].content_hash = fs->blocks[original_block].content_hash;
    
    fs->metrics.bytes_saved_cow += BLOCK_SIZE;
    
    return new_block;
}
// ===== DEDUP LOOKUP =====
int32_t fs_find_dedup_block(FileSystem *fs, const Hash *hash) {
    for (uint32_t i = 0; i < fs->dedup_count; i++) {
        uint32_t block_id = fs->dedup_table[i].block_id;

        if (hash_equals(&fs->blocks[block_id].content_hash, hash)) {
            return block_id;
            
        }
        
    }
    return -1;
}

bool fs_create_file(FileSystem *fs, const char *name, ImmutablePolicy policy) {
    if (!fs || !name) return false;

    uint32_t inode_id = fs_create_inode(fs, name, false);
    if (inode_id == 0) return false;

    return fs_set_immutable_policy(fs, inode_id, policy);
}

// Create inode
uint32_t fs_create_inode(FileSystem *fs, const char *filename, bool is_directory) {
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].inode_id == 0) {
            Inode *inode = &fs->inodes[i];
            inode->inode_id = i + 1;
            strncpy(inode->filename, filename, MAX_FILENAME - 1);
            inode->size = 0;
            inode->created_at = time(NULL);
            inode->modified_at = time(NULL);
            inode->accessed_at = time(NULL);
            inode->block_count = 0;
            inode->blocks = NULL;
            inode->current_version = 0;
            inode->version_count = 0;
            inode->versions = NULL;
            inode->immutable_policy = POLICY_NONE;
            inode->attr_count = 0;
            inode->is_directory = is_directory;
            inode->parent_inode = 0;
            
            fs->used_inodes++;
            fs->is_dirty = true;
            
            return inode->inode_id;
        }
    }
    return 0;
}

// Get inode
Inode* fs_get_inode(FileSystem *fs, uint32_t inode_id) {
    if (inode_id == 0 || inode_id > fs->total_inodes) return NULL;
    
    Inode *inode = &fs->inodes[inode_id - 1];
    if (inode->inode_id == 0) return NULL;
    
    inode->accessed_at = time(NULL);
    return inode;
}
// Get inode by filename
Inode* fs_get_inode_by_name(FileSystem *fs, const char *filename) {
    if (!fs || !filename) return NULL;

    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        Inode *inode = &fs->inodes[i];

        if (inode->inode_id != 0 &&
            strcmp(inode->filename, filename) == 0) {
            return inode;
        }
    }
    return NULL;
}

// Delete inode
void fs_delete_inode(FileSystem *fs, uint32_t inode_id) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return;
    
    // Check immutable policy
    if (inode->immutable_policy != POLICY_NONE) {
        printf("Cannot delete: inode is immutable\n");
        return;
    }
    
    // Free all blocks
    for (uint32_t i = 0; i < inode->block_count; i++) {
        fs_free_block(fs, inode->blocks[i]);
    }
    
    if (inode->blocks) {
        free(inode->blocks);
    }
    
    // Free versions
    if (inode->versions) {
        for (uint32_t v = 0; v < inode->version_count; v++) {
            if (inode->versions[v].blocks) {
                for (uint32_t b = 0; b < inode->versions[v].block_count; b++) {
                    fs_free_block(fs, inode->versions[v].blocks[b]);
                }
                free(inode->versions[v].blocks);
            }
        }
        free(inode->versions);
    }
    
    memset(inode, 0, sizeof(Inode));
    fs->used_inodes--;
    fs->is_dirty = true;
}

// Set immutable policy
bool fs_set_immutable_policy(FileSystem *fs, uint32_t inode_id, ImmutablePolicy policy) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;
    
    inode->immutable_policy = policy;
    if (policy != POLICY_NONE) {
        inode->immutable_since = time(NULL);
    }
    
    fs->is_dirty = true;
    return true;
}

// ===== REGISTER DEDUP BLOCK =====
void fs_register_dedup(FileSystem *fs, uint32_t block_id, const Hash *hash) {
    if (fs->dedup_count >= MAX_BLOCKS) return;

    fs->dedup_table[fs->dedup_count++].block_id = block_id;
    fs->blocks[block_id].is_deduplicated = true;
}

// Write file with strategy
bool fs_write_file(FileSystem *fs,
                   uint32_t inode_id,
                   const void *data,
                   uint64_t size,
                   WriteStrategy strategy)
{
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;

    // ===== IMMUTABLE CHECK =====
    if (inode->immutable_policy == POLICY_READ_ONLY ||
        inode->immutable_policy == POLICY_WORM) {
        printf("Cannot write: inode is immutable\n");
        return false;
    }

    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // ===== RESIZE BLOCK POINTER ARRAY =====
    inode->blocks = realloc(inode->blocks,
                             (inode->block_count + blocks_needed) * sizeof(uint32_t));
    if (!inode->blocks) return false;

    // ===== WRITE LOOP =====
    for (uint32_t i = 0; i < blocks_needed; i++) {

        uint8_t buffer[BLOCK_SIZE] = {0};
        uint64_t offset = i * BLOCK_SIZE;
        uint64_t to_write = (size - offset > BLOCK_SIZE)
                            ? BLOCK_SIZE
                            : (size - offset);

        memcpy(buffer, bytes + offset, to_write);

        // ===== DEDUP HASH =====
        Hash hash;
        fs_compute_hash(buffer, BLOCK_SIZE, &hash);

        uint32_t block_id = (uint32_t)-1;

        // ===== DEDUP LOOKUP =====
        
        for (uint32_t d = 0; d < fs->dedup_count; d++) {
            uint32_t existing = fs->dedup_table[d].block_id;
            if (hash_equals(&fs->blocks[existing].content_hash, &hash)) {
                block_id = existing;
                fs->blocks[block_id].ref_count++;
                fs->blocks[block_id].is_deduplicated = true;

                // ✅ Update metrics
                fs->metrics.blocks_deduplicated++;
                fs->metrics.bytes_saved_dedup += BLOCK_SIZE;

                break;
            }
        }

        // ===== NEW BLOCK REQUIRED =====
        if (block_id == (uint32_t)-1) {

            block_id = fs_allocate_block(fs, BLOCK_DATA);
            if (block_id == (uint32_t)-1) return false;

            fs_write_block(fs, block_id, buffer);

            // Register dedup entry
            fs->dedup_table[fs->dedup_count].block_id = block_id;
            fs->dedup_count++;

            if (strategy == STRATEGY_COW) {
                fs->blocks[block_id].is_cow = true;

            }
        }

        inode->blocks[inode->block_count++] = block_id;
    }

    inode->size += size;
    inode->modified_at = time(NULL);
    fs->is_dirty = true;
    // Auto-create version after write
    uint32_t new_version = fs_create_version(fs, inode->inode_id, "Auto-version from write");
    if (new_version == 0) {
        printf("Warning: failed to create auto-version for file %s\n", inode->filename);
}

    return true;
}
bool fs_write_file_api(FileSystem *fs, const char *filename, const void *data, uint64_t size, WriteStrategy strategy, Inode *out_inode) {
    // Find inode
    uint32_t inode_id = 0;
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].inode_id != 0 && strcmp(fs->inodes[i].filename, filename) == 0) {
            inode_id = fs->inodes[i].inode_id;
            break;
        }
    }
    if (inode_id == 0) return false;

    // Write
    bool result = fs_write_file(fs, inode_id, data, size, strategy);
    if (!result) return false;
    Inode *inode = fs_get_inode(fs, inode_id);  // <-- add this
    if (!inode) return false;

    // Return updated inode info
    if (out_inode) {
        *out_inode = *fs_get_inode(fs, inode_id);
    }


    return true;
}

// Read file
bool fs_read_file(FileSystem *fs, uint32_t inode_id, void *buffer, uint64_t *size) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode || !buffer || !size) return false;
    
    clock_t start = clock();
    
    uint8_t *data = (uint8_t *)buffer;
    uint64_t bytes_read = 0;
    
    for (uint32_t i = 0; i < inode->block_count; i++) {
        uint8_t block_buffer[BLOCK_SIZE];
        if (!fs_read_block(fs, inode->blocks[i], block_buffer)) {
            return false;
        }
        
        uint64_t to_copy = (inode->size - bytes_read > BLOCK_SIZE) ? BLOCK_SIZE : (inode->size - bytes_read);
        memcpy(data + bytes_read, block_buffer, to_copy);
        bytes_read += to_copy;
    }
    
    *size = bytes_read;
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    fs->metrics.avg_read_time = (fs->metrics.avg_read_time * (fs->metrics.total_reads - inode->block_count) + time_taken) / fs->metrics.total_reads;
    
    return true;
}

// Append to file
bool fs_append_file(FileSystem *fs, uint32_t inode_id, const void *data, uint64_t size) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;
    
    // Check immutable policy
    if (inode->immutable_policy == POLICY_READ_ONLY) {
        printf("Cannot append: inode is read-only\n");
        return false;
    }
    
    // Read existing data
    uint8_t *existing_data = (uint8_t *)malloc(inode->size + size);
    uint64_t existing_size = 0;
    
    if (inode->size > 0) {
        fs_read_file(fs, inode_id, existing_data, &existing_size);
    }
    
    // Append new data
    memcpy(existing_data + existing_size, data, size);
    
    // Write back
    bool result = fs_write_file(fs, inode_id, existing_data, existing_size + size, fs->default_strategy);
    
    free(existing_data);
   
    return result;
}

// Get metrics
PerformanceMetrics fs_get_metrics(FileSystem *fs) {
    return fs->metrics;
}

// Reset metrics
void fs_reset_metrics(FileSystem *fs) {
    memset(&fs->metrics, 0, sizeof(PerformanceMetrics));
}

// Get storage usage
uint64_t fs_get_storage_usage(FileSystem *fs) {
    return fs->used_blocks * BLOCK_SIZE;
}

// Get actual usage (accounting for dedup and CoW)
uint64_t fs_get_actual_usage(FileSystem *fs) {
    uint64_t actual = 0;
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].type != BLOCK_FREE && !fs->blocks[i].is_deduplicated) {
            actual += BLOCK_SIZE;
        }
    }
    return actual;
}

// Get deduplication ratio
double fs_get_dedup_ratio(FileSystem *fs) {
    uint64_t total = fs_get_storage_usage(fs);
    uint64_t actual = fs_get_actual_usage(fs);
    
    if (actual == 0) return 1.0;
    return (double)total / (double)actual;
}
// Count deduplicated blocks
uint32_t fs_count_dedup_blocks(FileSystem *fs) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].is_deduplicated) count++;
    }
    return count;
}

// List files
void fs_list_files(FileSystem *fs) {
    printf("\n=== File List ===\n");
    for (uint32_t i = 0; i < fs->total_inodes; i++) {
        if (fs->inodes[i].inode_id != 0) {
            Inode *inode = &fs->inodes[i];
            printf("ID: %u | Name: %s | Size: %lu bytes | Blocks: %u | Versions: %u\n",
                   inode->inode_id, inode->filename, inode->size, 
                   inode->block_count, inode->version_count);
        }
    }
}

// Save filesystem to disk
bool fs_save(FileSystem *fs) {
    FILE *f = fopen(fs->disk_file, "wb");
    if (!f) return false;
    
    // Write header
    fwrite(&fs->total_blocks, sizeof(uint32_t), 1, f);
    fwrite(&fs->used_blocks, sizeof(uint32_t), 1, f);
    fwrite(&fs->total_inodes, sizeof(uint32_t), 1, f);
    fwrite(&fs->used_inodes, sizeof(uint32_t), 1, f);
    
    // Write blocks metadata
    fwrite(fs->blocks, sizeof(BlockMetadata), fs->total_blocks, f);
    
    // Write block data
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].type != BLOCK_FREE && fs->block_data[i]) {
            fwrite(fs->block_data[i], BLOCK_SIZE, 1, f);
        }
    }
    
    // Write inodes (simplified - in production, handle dynamic arrays properly)
    fwrite(fs->inodes, sizeof(Inode), fs->total_inodes, f);
    
    // Write metrics
    fwrite(&fs->metrics, sizeof(PerformanceMetrics), 1, f);
    
    fclose(f);
    fs->is_dirty = false;
    return true;
}

// Load filesystem from disk
bool fs_load(FileSystem *fs, const char *disk_file) {
    FILE *f = fopen(disk_file, "rb");
    if (!f) return false;
    
    // Read header
    fread(&fs->total_blocks, sizeof(uint32_t), 1, f);
    fread(&fs->used_blocks, sizeof(uint32_t), 1, f);
    fread(&fs->total_inodes, sizeof(uint32_t), 1, f);
    fread(&fs->used_inodes, sizeof(uint32_t), 1, f);
    
    // Read blocks metadata
    fread(fs->blocks, sizeof(BlockMetadata), fs->total_blocks, f);
    
    // Read block data
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].type != BLOCK_FREE) {
            if (!fs->block_data[i]) {
                fs->block_data[i] = (uint8_t *)malloc(BLOCK_SIZE);
            }
            fread(fs->block_data[i], BLOCK_SIZE, 1, f);
        }
    }
    
    // Read inodes
    fread(fs->inodes, sizeof(Inode), fs->total_inodes, f);
    
    // Read metrics
    fread(&fs->metrics, sizeof(PerformanceMetrics), 1, f);
    
    fclose(f);
    fs->is_dirty = false;
    return true;
}