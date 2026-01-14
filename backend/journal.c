#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Begin a new transaction
void fs_journal_begin(FileSystem *fs) {
    // Transaction begins implicitly with first log entry
    fs->next_transaction_id++;
}

// Log an operation to the journal
void fs_journal_log(FileSystem *fs, const char *operation, uint32_t inode_id, 
                   uint32_t block_id, uint32_t old_value, uint32_t new_value) {
    if (fs->journal_head >= JOURNAL_SIZE) {
        // Journal is full - in production, flush to disk
        printf("Warning: Journal is full\n");
        return;
    }
    
    JournalEntry *entry = &fs->journal[fs->journal_head];
    entry->transaction_id = fs->next_transaction_id;
    entry->timestamp = time(NULL);
    strncpy(entry->operation, operation, 63);
    entry->inode_id = inode_id;
    entry->block_id = block_id;
    entry->old_value = old_value;
    entry->new_value = new_value;
    entry->committed = false;
    
    fs->journal_head++;
    fs->metrics.journal_entries++;
}

// Commit the current transaction
void fs_journal_commit(FileSystem *fs) {
    // Mark all entries in current transaction as committed
    for (uint32_t i = fs->journal_tail; i < fs->journal_head; i++) {
        if (fs->journal[i].transaction_id == fs->next_transaction_id) {
            fs->journal[i].committed = true;
        }
    }
    
    // Advance tail to remove committed entries
    while (fs->journal_tail < fs->journal_head && 
           fs->journal[fs->journal_tail].committed) {
        fs->journal_tail++;
    }
}

// Rollback the current transaction
void fs_journal_rollback(FileSystem *fs) {
    // Undo all uncommitted entries in reverse order
    for (int32_t i = fs->journal_head - 1; i >= (int32_t)fs->journal_tail; i--) {
        JournalEntry *entry = &fs->journal[i];
        
        if (entry->transaction_id == fs->next_transaction_id && !entry->committed) {
            // Undo the operation
            if (strcmp(entry->operation, "write_block") == 0) {
                // Restore old value (simplified)
                fs->blocks[entry->block_id].ref_count = entry->old_value;
            } else if (strcmp(entry->operation, "allocate_block") == 0) {
                // Free the block
                fs_free_block(fs, entry->block_id);
            } else if (strcmp(entry->operation, "free_block") == 0) {
                // Restore the block
                fs->blocks[entry->block_id].type = BLOCK_DATA;
                fs->blocks[entry->block_id].ref_count = entry->old_value;
            }
            
            // Remove entry
            fs->journal_head--;
        }
    }
}

// Recover from journal after crash
void fs_journal_recover(FileSystem *fs) {
    printf("Recovering from journal...\n");
    
    uint32_t recovered = 0;
    uint32_t rolled_back = 0;
    
    // Replay committed transactions
    for (uint32_t i = fs->journal_tail; i < fs->journal_head; i++) {
        JournalEntry *entry = &fs->journal[i];
        
        if (entry->committed) {
            // Replay the operation
            if (strcmp(entry->operation, "write_block") == 0) {
                fs->blocks[entry->block_id].ref_count = entry->new_value;
            }
            recovered++;
        } else {
            // Rollback uncommitted transaction
            if (strcmp(entry->operation, "allocate_block") == 0) {
                fs_free_block(fs, entry->block_id);
            }
            rolled_back++;
        }
    }
    
    // Clear journal
    fs->journal_head = 0;
    fs->journal_tail = 0;
    
    printf("Recovery complete: %u operations recovered, %u rolled back\n", 
           recovered, rolled_back);
}
