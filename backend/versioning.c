#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new version of a file
uint32_t fs_create_version(FileSystem *fs, uint32_t inode_id, const char *description) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return 0;
    
    // Allocate or expand versions array
    if (inode->versions == NULL) {
        inode->versions = (FileVersion *)malloc(sizeof(FileVersion));
        if (!inode->versions) return 0;
    } else {
        FileVersion *tmp = (FileVersion *)realloc(
            inode->versions,
            (inode->version_count + 1) * sizeof(FileVersion)
        );
        if (!tmp) return 0;
        inode->versions = tmp;
    }
    
    FileVersion *version = &inode->versions[inode->version_count];
    version->version_id = inode->version_count + 1;
    version->inode_id = inode_id;
    version->parent_version = inode->current_version;
    version->size = inode->size;
    version->block_count = inode->block_count;
    version->strategy = fs->default_strategy;
    version->is_snapshot_version = false;
    strncpy(version->description, description, 511);
    
    // Copy block references (NO ref_count change)
    version->blocks = (uint32_t *)malloc(inode->block_count * sizeof(uint32_t));
    if (!version->blocks) return 0;

    for (uint32_t i = 0; i < inode->block_count; i++) {
        version->blocks[i] = inode->blocks[i];
    }
    
    inode->version_count++;
    inode->current_version = version->version_id;
    
    fs->is_dirty = true;
    return version->version_id;
}

// Rollback to a specific version
bool fs_rollback_version(FileSystem *fs, uint32_t inode_id, uint32_t version_id) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;
    
    if (version_id == 0 || version_id > inode->version_count) return false;
    
    FileVersion *version = &inode->versions[version_id - 1];
    
    // Save block data BEFORE freeing
    uint8_t **saved_data = (uint8_t **)malloc(version->block_count * sizeof(uint8_t *));
    if (!saved_data) return false;
    
    for (uint32_t i = 0; i < version->block_count; i++) {
        saved_data[i] = (uint8_t *)malloc(BLOCK_SIZE);
        if (!saved_data[i]) {
            for (uint32_t j = 0; j < i; j++) free(saved_data[j]);
            free(saved_data);
            return false;
        }
        memcpy(saved_data[i], fs->block_data[version->blocks[i]], BLOCK_SIZE);
    }
    
    // STEP 1: Check if blocks are freed, allocate new if needed
    for (uint32_t i = 0; i < version->block_count; i++) {
        if (fs->blocks[version->blocks[i]].type == BLOCK_FREE) {
            // Block was freed, allocate new one
            uint32_t new_block = fs_allocate_block(fs, BLOCK_DATA);
            if (new_block == (uint32_t)-1) {
                for (uint32_t j = 0; j < version->block_count; j++) free(saved_data[j]);
                free(saved_data);
                return false;
            }
            // Restore data to new block
            memcpy(fs->block_data[new_block], saved_data[i], BLOCK_SIZE);
            fs->blocks[new_block].is_cow = true; 
            version->blocks[i] = new_block;
        } else {
            // Block exists, increment refcount
            fs->blocks[version->blocks[i]].ref_count++;
        }
    }
    
    // STEP 2: Decrement current blocks
    for (uint32_t i = 0; i < inode->block_count; i++) {
        fs_free_block(fs, inode->blocks[i]);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < version->block_count; i++) {
        free(saved_data[i]);
    }
    free(saved_data);
    
    if (inode->blocks) {
        free(inode->blocks);
    }
    
    inode->block_count = version->block_count;
    inode->blocks = (uint32_t *)malloc(version->block_count * sizeof(uint32_t));
    if (!inode->blocks) return false;
    
    // STEP 3: Copy block IDs
    for (uint32_t i = 0; i < version->block_count; i++) {
        inode->blocks[i] = version->blocks[i];
    }
    
    inode->size = version->size;
    inode->current_version = version_id;
    inode->modified_at = time(NULL);
    
    fs->is_dirty = true;
    return true;
}

