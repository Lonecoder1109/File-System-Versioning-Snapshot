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
    version->created_at = time(NULL);
    version->size = inode->size;
    version->block_count = inode->block_count;
    version->strategy = fs->default_strategy;
    version->tag_count = 0;
    version->attr_count = 0;
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
    
    // Free current blocks (decrement ref counts)
    for (uint32_t i = 0; i < inode->block_count; i++) {
        fs_free_block(fs, inode->blocks[i]);
    }
    
    // Restore from version
    if (inode->blocks) {
        free(inode->blocks);
    }
    
    inode->block_count = version->block_count;
    inode->blocks = (uint32_t *)malloc(version->block_count * sizeof(uint32_t));
    if (!inode->blocks) return false;
    
    for (uint32_t i = 0; i < version->block_count; i++) {
        inode->blocks[i] = version->blocks[i];
        fs->blocks[version->blocks[i]].ref_count++;
    }
    
    inode->size = version->size;
    inode->current_version = version_id;
    inode->modified_at = time(NULL);
    
    fs->is_dirty = true;
    return true;
}

// Add semantic tag to a version
bool fs_add_version_tag(FileSystem *fs, uint32_t inode_id, uint32_t version_id, 
                       const char *tag, const char *description) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;
    
    if (version_id == 0 || version_id > inode->version_count) return false;
    
    FileVersion *version = &inode->versions[version_id - 1];
    
    if (version->tag_count >= MAX_TAGS_PER_VERSION) return false;
    
    SemanticTag *new_tag = &version->tags[version->tag_count];
    strncpy(new_tag->tag, tag, MAX_TAG_LENGTH - 1);
    strncpy(new_tag->description, description, 255);
    new_tag->created_at = time(NULL);
    
    version->tag_count++;
    fs->is_dirty = true;
    
    return true;
}

// Find versions by semantic tag
FileVersion* fs_find_versions_by_tag(FileSystem *fs, uint32_t inode_id, const char *tag, uint32_t *count) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) {
        *count = 0;
        return NULL;
    }
    
    *count = 0;
    FileVersion *results = (FileVersion *)malloc(inode->version_count * sizeof(FileVersion));
    if (!results) return NULL;
    
    for (uint32_t v = 0; v < inode->version_count; v++) {
        FileVersion *version = &inode->versions[v];
        
        for (uint32_t t = 0; t < version->tag_count; t++) {
            if (strcmp(version->tags[t].tag, tag) == 0) {
                results[*count] = *version;
                (*count)++;
                break;
            }
        }
    }
    
    return results;
}

// Set extended attribute on inode
bool fs_set_extended_attribute(FileSystem *fs, uint32_t inode_id, const char *key, const char *value) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return false;
    
    // Check if attribute already exists
    for (uint32_t i = 0; i < inode->attr_count; i++) {
        if (strcmp(inode->attributes[i].key, key) == 0) {
            strncpy(inode->attributes[i].value, value, 255);
            fs->is_dirty = true;
            return true;
        }
    }
    
    if (inode->attr_count >= 20) return false;
    
    ExtendedAttribute *attr = &inode->attributes[inode->attr_count];
    strncpy(attr->key, key, 63);
    strncpy(attr->value, value, 255);
    
    inode->attr_count++;
    fs->is_dirty = true;
    
    return true;
}

// Get extended attribute from inode
char* fs_get_extended_attribute(FileSystem *fs, uint32_t inode_id, const char *key) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return NULL;
    
    for (uint32_t i = 0; i < inode->attr_count; i++) {
        if (strcmp(inode->attributes[i].key, key) == 0) {
            return inode->attributes[i].value;
        }
    }
    
    return NULL;
}

// List all versions of a file
void fs_list_versions(FileSystem *fs, uint32_t inode_id) {
    Inode *inode = fs_get_inode(fs, inode_id);
    if (!inode) return;
    
    printf("\n=== Versions for %s (ID: %u) ===\n", inode->filename, inode_id);
    for (uint32_t v = 0; v < inode->version_count; v++) {
        FileVersion *version = &inode->versions[v];
        printf("Version %u | Size: %lu bytes | Blocks: %u | Tags: %u | %s\n",
               version->version_id, version->size, version->block_count,
               version->tag_count, version->description);
        
        for (uint32_t t = 0; t < version->tag_count; t++) {
            printf("  Tag: %s - %s\n", version->tags[t].tag, version->tags[t].description);
        }
    }
}