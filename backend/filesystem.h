#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Configuration constants
#define BLOCK_SIZE 4096
#define MAX_FILENAME 256
#define MAX_PATH 1024
#define MAX_BLOCKS 100000
#define MAX_INODES 10000
#define MAX_SNAPSHOTS 1000
#define MAX_VERSIONS 100
#define MAX_TAG_LENGTH 128
#define MAX_TAGS_PER_VERSION 10
#define HASH_SIZE 32
#define JOURNAL_SIZE 10000

// Block types
typedef enum {
    BLOCK_FREE = 0,
    BLOCK_DATA = 1,
    BLOCK_INODE = 2,
    BLOCK_METADATA = 3,
    BLOCK_SNAPSHOT = 4,
    BLOCK_JOURNAL = 5,
    BLOCK_BTREE = 6,
    BLOCK_DEDUP = 7
} BlockType;

// Write strategies
typedef enum {
    STRATEGY_COW = 0,  // Copy-on-Write
    STRATEGY_ROW = 1   // Redirect-on-Write
} WriteStrategy;

// Snapshot granularity
typedef enum {
    GRANULARITY_FINE = 0,    // Every few operations
    GRANULARITY_MEDIUM = 1,  // Moderate frequency
    GRANULARITY_COARSE = 2   // Infrequent
} SnapshotGranularity;

// Immutable zone policy
typedef enum {
    POLICY_NONE = 0,
    POLICY_READ_ONLY = 1,
    POLICY_APPEND_ONLY = 2,
    POLICY_WORM = 3  // Write Once Read Many
} ImmutablePolicy;

// Hash for deduplication
typedef struct {
    uint8_t hash[HASH_SIZE];
} Hash;

// Block metadata
typedef struct {
    uint32_t block_id;
    BlockType type;
    uint32_t ref_count;
    uint32_t next_block;
    Hash content_hash;
    time_t created_at;
    time_t modified_at;
    bool is_cow;
    bool is_deduplicated;
    uint32_t original_block;  // For CoW blocks
} BlockMetadata;

// Extended attributes for contextual versioning
typedef struct {
    char key[64];
    char value[256];
} ExtendedAttribute;

// Semantic tag for versioning
typedef struct {
    char tag[MAX_TAG_LENGTH];
    char description[256];
    time_t created_at;
} SemanticTag;

// File version
typedef struct {
    uint32_t version_id;
    uint32_t inode_id;
    uint32_t parent_version;
    time_t created_at;
    uint64_t size;
    uint32_t block_count;
    uint32_t *blocks;
    WriteStrategy strategy;
    
    // Contextual versioning
    SemanticTag tags[MAX_TAGS_PER_VERSION];
    uint32_t tag_count;
    ExtendedAttribute attributes[10];
    uint32_t attr_count;
    
    char description[512];
    bool is_snapshot_version;
} FileVersion;

// Inode structure
typedef struct {
    uint32_t inode_id;
    char filename[MAX_FILENAME];
    uint64_t size;
    time_t created_at;
    time_t modified_at;
    time_t accessed_at;
    
    uint32_t block_count;
    uint32_t *blocks;
    
    // Versioning
    uint32_t current_version;
    uint32_t version_count;
    FileVersion *versions;
    
    // Immutable zones
    ImmutablePolicy immutable_policy;
    time_t immutable_since;
    
    // Extended attributes
    ExtendedAttribute attributes[20];
    uint32_t attr_count;
    
    bool is_directory;
    uint32_t parent_inode;
} Inode;

// Snapshot metadata
typedef struct {
    uint32_t snapshot_id;
    char name[MAX_FILENAME];
    time_t created_at;
    uint64_t total_size;
    uint32_t inode_count;
    uint32_t *inodes;
    
    // Hierarchical grouping
    uint32_t parent_snapshot;
    uint32_t *child_snapshots;
    uint32_t child_count;
    char group_name[MAX_FILENAME];
    
    // Adaptive granularity
    SnapshotGranularity granularity;
    uint32_t operations_since_last;
    double importance_score;
    
    // Metadata
    char description[512];
    SemanticTag tags[MAX_TAGS_PER_VERSION];
    uint32_t tag_count;
    
    bool is_trimmed;
    uint32_t ref_count;
} Snapshot;

// Journal entry for crash recovery
typedef struct {
    uint64_t transaction_id;
    time_t timestamp;
    char operation[64];
    uint32_t inode_id;
    uint32_t block_id;
    uint32_t old_value;
    uint32_t new_value;
    bool committed;
} JournalEntry;

// Deduplication entry
typedef struct {
    Hash content_hash;
    uint32_t block_id;
    uint32_t ref_count;
    uint64_t size;
    time_t first_seen;
} DedupEntry;

// Performance metrics
typedef struct {
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t total_snapshots;
    uint64_t total_rollbacks;
    uint64_t blocks_allocated;
    uint64_t blocks_freed;
    uint64_t blocks_deduplicated;
    uint64_t bytes_saved_dedup;
    uint64_t bytes_saved_cow;
    double avg_snapshot_time;
    double avg_rollback_time;
    double avg_write_time;
    double avg_read_time;
    uint64_t journal_entries;
} PerformanceMetrics;

// Main filesystem structure
typedef struct {
    // Block management
    BlockMetadata *blocks;
    uint32_t total_blocks;
    uint32_t used_blocks;
    uint8_t **block_data;
    
    // Inode management
    Inode *inodes;
    uint32_t total_inodes;
    uint32_t used_inodes;
    
    // Snapshot management
    Snapshot *snapshots;
    uint32_t snapshot_count;
    
    // Journal
    JournalEntry *journal;
    uint32_t journal_head;
    uint32_t journal_tail;
    uint64_t next_transaction_id;
    
    // Deduplication
    DedupEntry *dedup_table;
    uint32_t dedup_count;
    
    // Performance metrics
    PerformanceMetrics metrics;
    
    // Configuration
    WriteStrategy default_strategy;
    SnapshotGranularity default_granularity;
    bool auto_snapshot_enabled;
    uint32_t auto_snapshot_threshold;
    
    // Persistence
    char disk_file[MAX_PATH];
    bool is_dirty;
} FileSystem;

// Function declarations

// Initialization and cleanup
FileSystem* fs_create(const char *disk_file, uint32_t total_blocks, uint32_t total_inodes);
void fs_destroy(FileSystem *fs);
bool fs_load(FileSystem *fs, const char *disk_file);
bool fs_save(FileSystem *fs);
void fs_format(FileSystem *fs);

// Block management
uint32_t fs_allocate_block(FileSystem *fs, BlockType type);
void fs_free_block(FileSystem *fs, uint32_t block_id);
bool fs_read_block(FileSystem *fs, uint32_t block_id, void *buffer);
bool fs_write_block(FileSystem *fs, uint32_t block_id, const void *buffer);
uint32_t fs_cow_block(FileSystem *fs, uint32_t original_block);
void fs_compute_hash(const void *data, size_t size, Hash *hash);
bool hash_equals(const Hash *h1, const Hash *h2);

// Inode management
uint32_t fs_create_inode(FileSystem *fs, const char *filename, bool is_directory);
void fs_delete_inode(FileSystem *fs, uint32_t inode_id);
Inode* fs_get_inode(FileSystem *fs, uint32_t inode_id);
bool fs_set_immutable_policy(FileSystem *fs, uint32_t inode_id, ImmutablePolicy policy);

// File operations
bool fs_write_file(FileSystem *fs, uint32_t inode_id, const void *data, uint64_t size, WriteStrategy strategy);
bool fs_read_file(FileSystem *fs, uint32_t inode_id, void *buffer, uint64_t *size);
bool fs_append_file(FileSystem *fs, uint32_t inode_id, const void *data, uint64_t size);
bool fs_truncate_file(FileSystem *fs, uint32_t inode_id, uint64_t new_size);

// Versioning
uint32_t fs_create_version(FileSystem *fs, uint32_t inode_id, const char *description);
bool fs_rollback_version(FileSystem *fs, uint32_t inode_id, uint32_t version_id);
bool fs_add_version_tag(FileSystem *fs, uint32_t inode_id, uint32_t version_id, const char *tag, const char *description);
FileVersion* fs_find_versions_by_tag(FileSystem *fs, uint32_t inode_id, const char *tag, uint32_t *count);
bool fs_set_extended_attribute(FileSystem *fs, uint32_t inode_id, const char *key, const char *value);
char* fs_get_extended_attribute(FileSystem *fs, uint32_t inode_id, const char *key);

// Snapshot management
uint32_t fs_create_snapshot(FileSystem *fs, const char *name, const char *description);
bool fs_delete_snapshot(FileSystem *fs, uint32_t snapshot_id);
bool fs_rollback_snapshot(FileSystem *fs, uint32_t snapshot_id);
bool fs_trim_snapshot(FileSystem *fs, uint32_t snapshot_id);
bool fs_add_snapshot_tag(FileSystem *fs, uint32_t snapshot_id, const char *tag, const char *description);
Snapshot* fs_find_snapshots_by_tag(FileSystem *fs, const char *tag, uint32_t *count);

// Hierarchical snapshot grouping
bool fs_create_snapshot_group(FileSystem *fs, const char *group_name, uint32_t *snapshot_ids, uint32_t count);
bool fs_set_snapshot_parent(FileSystem *fs, uint32_t snapshot_id, uint32_t parent_id);
Snapshot** fs_get_snapshot_hierarchy(FileSystem *fs, uint32_t root_snapshot_id, uint32_t *count);

// Adaptive snapshot granularity
void fs_update_snapshot_importance(FileSystem *fs, uint32_t snapshot_id);
bool fs_should_create_snapshot(FileSystem *fs);
void fs_adjust_granularity(FileSystem *fs, SnapshotGranularity granularity);

// Deduplication
uint32_t fs_deduplicate_block(FileSystem *fs, const void *data, uint64_t size);
void fs_scan_and_deduplicate(FileSystem *fs);
uint64_t fs_get_dedup_savings(FileSystem *fs);

// Journal
void fs_journal_begin(FileSystem *fs);
void fs_journal_log(FileSystem *fs, const char *operation, uint32_t inode_id, uint32_t block_id, uint32_t old_value, uint32_t new_value);
void fs_journal_commit(FileSystem *fs);
void fs_journal_rollback(FileSystem *fs);
void fs_journal_recover(FileSystem *fs);

// Performance and metrics
PerformanceMetrics fs_get_metrics(FileSystem *fs);
void fs_reset_metrics(FileSystem *fs);
void fs_print_metrics(FileSystem *fs);

// Utility functions
void fs_list_files(FileSystem *fs);
void fs_list_snapshots(FileSystem *fs);
void fs_list_versions(FileSystem *fs, uint32_t inode_id);
uint64_t fs_get_storage_usage(FileSystem *fs);
uint64_t fs_get_actual_usage(FileSystem *fs);
double fs_get_dedup_ratio(FileSystem *fs);

#endif // FILESYSTEM_H
