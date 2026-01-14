# Advanced File System Simulator

A comprehensive file system prototype with **React frontend** and **C backend**, featuring advanced snapshot management, versioning, Copy-on-Write (CoW), Redirect-on-Write (RoW), deduplication, and journaling.

## 🚀 Features

### Core File System Operations
- **Block Management**: Efficient 4KB block allocation with multiple block types
- **Inode Management**: Support for files and directories with extended attributes
- **Write Strategies**: 
  - Copy-on-Write (CoW) for versioning and snapshots
  - Redirect-on-Write (RoW) for performance optimization

### Advanced Features

#### 1. **Adaptive Snapshot Granularity**
- Heuristic-based dynamic snapshot scheduling
- Automatic importance scoring
- Configurable granularity levels (Fine, Medium, Coarse)
- Smart snapshot trimming to optimize storage

#### 2. **Contextual Versioning**
- Semantic tagging for versions and snapshots
- Extended attributes for metadata
- Filtered retrieval by tags
- Version timeline and rollback capabilities

#### 3. **Integrated Block-Level Deduplication**
- Content-based hashing (SHA-256 style)
- Automatic duplicate detection
- Reference counting for shared blocks
- Real-time space savings tracking

#### 4. **Immutable Zones**
- Policy-driven data integrity
- Support for: Read-Only, Append-Only, WORM (Write Once Read Many)
- Compliance and audit trail support

#### 5. **Hierarchical Snapshot Grouping**
- Parent-child snapshot relationships
- Logical organization by groups
- Targeted rollbacks to specific snapshot trees

#### 6. **Journaling**
- Transaction-based operations
- Crash recovery support
- Commit/rollback capabilities
- Operation logging for audit

## 📁 Project Structure

```
OSLab_EL/
├── backend/                 # C Backend
│   ├── filesystem.h         # Core data structures and function declarations
│   ├── filesystem.c         # Block and inode management
│   ├── snapshot.c           # Snapshot operations
│   ├── versioning.c         # File versioning
│   ├── dedup.c             # Deduplication engine
│   ├── journal.c           # Journaling system
│   ├── btree.c             # B-tree for metadata indexing
│   └── main.c              # HTTP server (REST API)
│
├── src/                     # React Frontend
│   ├── components/
│   │   ├── BlockManager.jsx           # Block visualization and management
│   │   ├── FileOperations.jsx         # File create/write/read operations
│   │   ├── SnapshotManager.jsx        # Snapshot creation and management
│   │   ├── VersionManager.jsx         # Version control interface
│   │   ├── PerformanceMetrics.jsx     # Charts and performance data
│   │   └── FileSystemVisualizer.jsx   # Visual block map and hierarchy
│   ├── App.jsx              # Main application
│   ├── main.jsx            # React entry point
│   └── index.css           # Premium design system
│
├── index.html              # HTML entry point
├── package.json            # Dependencies
├── vite.config.js          # Vite configuration
└── README.md              # This file
```

## 🛠️ Installation & Setup

### Prerequisites
- **Node.js** (v18+)
- **npm** or **yarn**
- **GCC** compiler (for C backend)
- **Windows** (or modify compilation for Linux/Mac)

### Step 1: Install Frontend Dependencies

```bash
npm install
```

### Step 2: Compile C Backend

#### On Windows (using GCC/MinGW):
```bash
gcc -o backend/fs_backend.exe backend/main.c backend/filesystem.c backend/snapshot.c backend/versioning.c backend/btree.c backend/dedup.c backend/journal.c -lws2_32 -lm
```

#### On Linux/Mac:
```bash
gcc -o backend/fs_backend backend/main.c backend/filesystem.c backend/snapshot.c backend/versioning.c backend/btree.c backend/dedup.c backend/journal.c -lm
```

### Step 3: Start the Backend Server

```bash
# Windows
backend\fs_backend.exe

# Linux/Mac
./backend/fs_backend
```

The backend will start on `http://localhost:8080`

### Step 4: Start the Frontend

```bash
npm run dev
```

The frontend will start on `http://localhost:3000`

## 🎮 Usage Guide

### Creating Files
1. Navigate to the **File Operations** tab
2. Click **Create File**
3. Enter a filename and click **Create**

### Writing to Files
1. Select a file from the list
2. Click the **Edit** button
3. Choose write strategy (CoW or RoW)
4. Enter data and click **Write**

### Creating Snapshots
1. Go to the **Snapshots** tab
2. Click **Create Snapshot**
3. Enter name and description
4. The snapshot captures the current file system state

### Managing Versions
1. Navigate to **Versions** tab
2. Select a file to view its version history
3. Add semantic tags to versions
4. Rollback to previous versions as needed

### Viewing Performance
1. Go to **Performance** tab
2. View real-time metrics and charts
3. Monitor storage efficiency and operation timings

### Block Visualization
1. **Overview** tab shows the block allocation map
2. Hover over blocks to see details
3. Filter by block type (Free, Used, CoW, Dedup)
4. Switch between Block View and Hierarchy View

## 🔧 API Endpoints

The C backend exposes the following REST API:

### System Status
- `GET /api/status` - Get system metrics and statistics

### File Operations
- `GET /api/files` - List all files
- `POST /api/files?name=<filename>` - Create a new file
- `POST /api/files/write?id=<id>&strategy=<cow|row>` - Write to file

### Block Management
- `GET /api/blocks` - List all blocks with metadata

### Snapshots
- `GET /api/snapshots` - List all snapshots
- `POST /api/snapshots?name=<name>&description=<desc>` - Create snapshot
- `POST /api/snapshots/rollback?id=<id>` - Rollback to snapshot
- `DELETE /api/snapshots?id=<id>` - Delete snapshot

### Versioning
- `GET /api/versions?fileId=<id>` - Get file versions
- `POST /api/versions?fileId=<id>&description=<desc>` - Create version
- `POST /api/versions/tag?fileId=<id>&versionId=<vid>&tag=<tag>` - Add tag

## 📊 Performance Metrics

The simulator tracks:
- **Total Reads/Writes**: Operation counts
- **Average Timings**: Read, write, snapshot, rollback times
- **Block Statistics**: Allocated, freed, deduplicated
- **Space Savings**: CoW and deduplication efficiency
- **Storage Utilization**: Block and inode usage percentages

## 🎨 Design Features

### Premium UI/UX
- **Dark Mode** with vibrant gradients
- **Glassmorphism** effects
- **Smooth animations** and micro-interactions
- **Interactive charts** using Recharts
- **Responsive design** for all screen sizes

### Visual Elements
- Real-time block allocation map
- Color-coded block types
- Progress bars for utilization
- Timeline view for versions
- Hierarchical snapshot tree

## 🧪 Testing Scenarios

### Scenario 1: Basic File Operations
1. Create multiple files
2. Write data using different strategies
3. Observe block allocation in Block Manager

### Scenario 2: Snapshot & Rollback
1. Create files and write data
2. Take a snapshot
3. Modify files
4. Rollback to snapshot
5. Verify data restoration

### Scenario 3: Deduplication
1. Create files with duplicate content
2. Run deduplication scan
3. Observe space savings in metrics

### Scenario 4: Versioning
1. Create a file
2. Make multiple edits
3. Tag important versions
4. Rollback to specific version

## 🔬 Simulation Objectives

### Storage Overhead vs. Benefit
- Compare raw storage vs. actual usage
- Measure deduplication ratio
- Track CoW efficiency

### Backup/Rollback Efficiency
- Snapshot creation time
- Rollback operation time
- Storage overhead per snapshot

### Performance Under Workloads
- Sequential writes (RoW advantage)
- Random writes (CoW advantage)
- Read performance
- Mixed workload scenarios

## 🚧 Limitations & Future Work

### Current Limitations
- In-memory storage (data lost on restart)
- Simplified persistence (basic save/load)
- Single-threaded backend
- No authentication/authorization

### Future Enhancements
- Persistent storage with proper disk layout
- Multi-threaded request handling
- Network file system support
- Advanced B+ tree implementation
- Compression support
- Encryption for immutable zones

## 📝 Technical Details

### Block Size
- Fixed 4KB blocks (configurable in `filesystem.h`)

### Data Structures
- **B-tree** for metadata indexing
- **Hash table** for deduplication
- **Circular buffer** for journal
- **Reference counting** for shared blocks

### Algorithms
- **CoW**: Copy blocks before modification
- **RoW**: Allocate new blocks, update pointers
- **Deduplication**: Content-based hashing with reference counting
- **Journaling**: Write-ahead logging with commit/rollback

## 🤝 Contributing

This is an educational project for OS Lab. Feel free to:
- Report bugs
- Suggest features
- Improve documentation
- Optimize algorithms

## 📄 License

Educational use only - OS Lab Project

## 👥 Authors

- **Fathima** - Initial implementation

## 🙏 Acknowledgments

- Operating Systems concepts from course materials
- File system design patterns from industry research
- React and modern web development best practices

---

**Note**: This is a simulation for educational purposes. Not intended for production use.
