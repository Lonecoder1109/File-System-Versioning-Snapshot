# System Architecture

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     USER INTERFACE                          │
│                   (Web Browser)                             │
│                  http://localhost:3000                      │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ HTTP Requests
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   REACT FRONTEND                            │
│                     (Vite + React)                          │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ File Ops     │  │ Block Mgr    │  │ Snapshots    │     │
│  │ Component    │  │ Component    │  │ Component    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Versions     │  │ Performance  │  │ Visualizer   │     │
│  │ Component    │  │ Component    │  │ Component    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ REST API Calls
                         │ (JSON over HTTP)
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   C BACKEND SERVER                          │
│                  http://localhost:8080                      │
├─────────────────────────────────────────────────────────────┤
│                    HTTP Server                              │
│              (Socket-based REST API)                        │
├─────────────────────────────────────────────────────────────┤
│  API Endpoints:                                             │
│  • GET  /api/status        • GET  /api/files               │
│  • POST /api/files         • POST /api/files/write         │
│  • GET  /api/blocks        • GET  /api/snapshots           │
│  • POST /api/snapshots     • POST /api/versions            │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Function Calls
                         │
┌────────────────────────▼────────────────────────────────────┐
│              FILE SYSTEM CORE MODULES                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │           filesystem.c - Core Operations            │   │
│  │  • Block Allocation/Deallocation                    │   │
│  │  • Inode Management                                 │   │
│  │  • File Read/Write (CoW & RoW)                      │   │
│  │  • Persistence (Save/Load)                          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          snapshot.c - Snapshot Management           │   │
│  │  • Snapshot Creation/Deletion                       │   │
│  │  • Hierarchical Grouping                            │   │
│  │  • Adaptive Granularity                             │   │
│  │  • Semantic Tagging                                 │   │
│  │  • Rollback Operations                              │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         versioning.c - Version Control              │   │
│  │  • Version Creation                                 │   │
│  │  • Semantic Tags                                    │   │
│  │  • Extended Attributes                              │   │
│  │  • Version Rollback                                 │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │           dedup.c - Deduplication Engine            │   │
│  │  • Content Hashing                                  │   │
│  │  • Duplicate Detection                              │   │
│  │  • Reference Counting                               │   │
│  │  • Space Savings Tracking                           │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          journal.c - Journaling System              │   │
│  │  • Transaction Management                           │   │
│  │  • Operation Logging                                │   │
│  │  • Commit/Rollback                                  │   │
│  │  • Crash Recovery                                   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │            btree.c - B-Tree Indexing                │   │
│  │  • Metadata Indexing                                │   │
│  │  • Fast Lookups                                     │   │
│  │  • Balanced Tree Operations                         │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Data Structures
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   DATA STRUCTURES                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  FileSystem                                                 │
│  ├── BlockMetadata[] (1000 blocks)                         │
│  ├── Inode[] (100 inodes)                                  │
│  ├── Snapshot[] (1000 snapshots)                           │
│  ├── JournalEntry[] (10000 entries)                        │
│  ├── DedupEntry[] (hash table)                             │
│  └── PerformanceMetrics                                    │
│                                                             │
│  Block (4KB each)                                          │
│  ├── Type (FREE, DATA, INODE, etc.)                        │
│  ├── Reference Count                                       │
│  ├── Content Hash                                          │
│  ├── CoW Flag                                              │
│  └── Dedup Flag                                            │
│                                                             │
│  Inode                                                     │
│  ├── Filename                                              │
│  ├── Size                                                  │
│  ├── Block Pointers[]                                      │
│  ├── Versions[]                                            │
│  ├── Extended Attributes[]                                 │
│  └── Immutable Policy                                      │
│                                                             │
│  Snapshot                                                  │
│  ├── Name & Description                                    │
│  ├── Inode References[]                                    │
│  ├── Parent/Child Links                                    │
│  ├── Semantic Tags[]                                       │
│  ├── Importance Score                                      │
│  └── Granularity Level                                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow

### Write Operation (Copy-on-Write)
```
User Input → Frontend → API Call → Backend
                                      ↓
                            Check Immutable Policy
                                      ↓
                            Journal: Begin Transaction
                                      ↓
                            Copy Original Blocks (CoW)
                                      ↓
                            Write New Data
                                      ↓
                            Update Inode Pointers
                                      ↓
                            Journal: Commit
                                      ↓
                            Update Metrics
                                      ↓
Frontend ← JSON Response ← Return Success
```

### Snapshot Creation
```
User Input → Frontend → API Call → Backend
                                      ↓
                            Capture Current State
                                      ↓
                            For Each Inode:
                            ├── Copy Inode ID
                            └── Increment Block RefCount
                                      ↓
                            Calculate Importance Score
                                      ↓
                            Add to Snapshot Array
                                      ↓
                            Update Metrics
                                      ↓
Frontend ← JSON Response ← Return Snapshot ID
```

### Deduplication Scan
```
User Trigger → Frontend → API Call → Backend
                                       ↓
                            For Each Data Block:
                            ├── Compute Hash
                            └── Store in Hash Table
                                       ↓
                            Find Duplicates:
                            ├── Compare Hashes
                            └── Merge References
                                       ↓
                            Free Duplicate Blocks
                                       ↓
                            Update Savings Metrics
                                       ↓
Frontend ← JSON Response ← Return Results
```

## Component Interaction

```
┌─────────────────────────────────────────────────────────┐
│                      App.jsx                            │
│                   (Main Container)                      │
│  ┌───────────────────────────────────────────────────┐ │
│  │  State Management:                                │ │
│  │  • systemStatus                                   │ │
│  │  • files[]                                        │ │
│  │  • blocks[]                                       │ │
│  │  • snapshots[]                                    │ │
│  │  • activeTab                                      │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Auto-Refresh Timer (2s interval)                 │ │
│  │  ├── fetchStatus()                                │ │
│  │  ├── fetchFiles()                                 │ │
│  │  ├── fetchBlocks()                                │ │
│  │  └── fetchSnapshots()                             │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Child Components (Props):                        │ │
│  │  ├── FileSystemVisualizer                         │ │
│  │  ├── BlockManager                                 │ │
│  │  ├── FileOperations                               │ │
│  │  ├── SnapshotManager                              │ │
│  │  ├── VersionManager                               │ │
│  │  └── PerformanceMetrics                           │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

## Technology Stack

```
┌─────────────────────────────────────────────────────────┐
│                    FRONTEND                             │
├─────────────────────────────────────────────────────────┤
│  • React 18.2.0          - UI Framework                 │
│  • Vite 5.0.0            - Build Tool                   │
│  • Recharts 2.10.0       - Charts Library               │
│  • Lucide React 0.300.0  - Icon Library                 │
│  • Vanilla CSS           - Styling                      │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                     BACKEND                             │
├─────────────────────────────────────────────────────────┤
│  • C Language            - Core Implementation          │
│  • GCC Compiler          - Compilation                  │
│  • Winsock2 (Windows)    - Networking                   │
│  • Standard C Library    - Data Structures              │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                  COMMUNICATION                          │
├─────────────────────────────────────────────────────────┤
│  • HTTP/1.1              - Protocol                     │
│  • JSON                  - Data Format                  │
│  • REST API              - Architecture                 │
│  • CORS Enabled          - Cross-Origin                 │
└─────────────────────────────────────────────────────────┘
```

This architecture provides:
- **Separation of Concerns**: Frontend and backend are independent
- **Scalability**: Easy to add new features
- **Maintainability**: Modular design
- **Performance**: Efficient data structures
- **Reliability**: Journaling and error handling
- **Usability**: Real-time updates and visual feedback
