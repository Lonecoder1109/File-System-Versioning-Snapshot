#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Deduplicate a block of data
uint32_t fs_deduplicate_block(FileSystem *fs, const void *data, uint64_t size) {
    Hash content_hash;
    fs_compute_hash(data, size, &content_hash);
    
    // Check if this content already exists
    for (uint32_t i = 0; i < fs->dedup_count; i++) {
        if (hash_equals(&fs->dedup_table[i].content_hash, &content_hash)) {
            // Found duplicate - increment ref count and return existing block
            fs->dedup_table[i].ref_count++;
            fs->blocks[fs->dedup_table[i].block_id].ref_count++;
            fs->metrics.blocks_deduplicated++;
            fs->metrics.bytes_saved_dedup += size;
            
            return fs->dedup_table[i].block_id;
        }
    }
    
    // Not found - allocate new block
    uint32_t new_block = fs_allocate_block(fs, BLOCK_DATA);
    if (new_block == (uint32_t)-1) return (uint32_t)-1;
    
    // Write data to block
    uint8_t buffer[BLOCK_SIZE] = {0};
    memcpy(buffer, data, size < BLOCK_SIZE ? size : BLOCK_SIZE);
    fs_write_block(fs, new_block, buffer);
    
    // Add to dedup table
    if (fs->dedup_count < MAX_BLOCKS) {
        DedupEntry *entry = &fs->dedup_table[fs->dedup_count];
        entry->content_hash = content_hash;
        entry->block_id = new_block;
        entry->ref_count = 1;
        entry->size = size;
        entry->first_seen = time(NULL);
        
        fs->blocks[new_block].is_deduplicated = true;
        fs->blocks[new_block].content_hash = content_hash;
        
        fs->dedup_count++;
    }
    
    return new_block;
}

// Scan entire filesystem and deduplicate blocks
void fs_scan_and_deduplicate(FileSystem *fs) {
    printf("Starting deduplication scan...\n");
    
    uint64_t blocks_before = fs->used_blocks;
    uint64_t bytes_saved = 0;
    
    // Build hash table of all blocks
    typedef struct {
        Hash hash;
        uint32_t block_id;
        uint32_t ref_count;
    } BlockHash;
    
    BlockHash *block_hashes = (BlockHash *)malloc(fs->total_blocks * sizeof(BlockHash));
    uint32_t hash_count = 0;
    
    // First pass: compute hashes for all data blocks
    for (uint32_t i = 0; i < fs->total_blocks; i++) {
        if (fs->blocks[i].type == BLOCK_DATA && fs->block_data[i]) {
            fs_compute_hash(fs->block_data[i], BLOCK_SIZE, &block_hashes[hash_count].hash);
            block_hashes[hash_count].block_id = i;
            block_hashes[hash_count].ref_count = fs->blocks[i].ref_count;
            hash_count++;
        }
    }
    
    // Second pass: find duplicates
    for (uint32_t i = 0; i < hash_count; i++) {
        for (uint32_t j = i + 1; j < hash_count; j++) {
            if (hash_equals(&block_hashes[i].hash, &block_hashes[j].hash)) {
                // Found duplicate
                uint32_t keep_block = block_hashes[i].block_id;
                uint32_t dup_block = block_hashes[j].block_id;
                
                // Update all inodes that reference the duplicate block
                for (uint32_t k = 0; k < fs->total_inodes; k++) {
                    Inode *inode = &fs->inodes[k];
                    if (inode->inode_id == 0) continue;
                    
                    for (uint32_t b = 0; b < inode->block_count; b++) {
                        if (inode->blocks[b] == dup_block) {
                            inode->blocks[b] = keep_block;
                            fs->blocks[keep_block].ref_count++;
                        }
                    }
                }
                
                // Free the duplicate block
                fs_free_block(fs, dup_block);
                bytes_saved += BLOCK_SIZE;
                
                // Mark as processed
                block_hashes[j].block_id = (uint32_t)-1;
            }
        }
    }
    
    free(block_hashes);
    
    uint64_t blocks_after = fs->used_blocks;
    fs->metrics.bytes_saved_dedup += bytes_saved;
    
    printf("Deduplication complete: %lu blocks freed, %lu bytes saved\n", 
           blocks_before - blocks_after, bytes_saved);
}

// Get total bytes saved through deduplication
uint64_t fs_get_dedup_savings(FileSystem *fs) {
    return fs->metrics.bytes_saved_dedup;
}
