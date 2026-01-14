# Quick Start Guide - Advanced File System Simulator

## 🚀 Get Started in 3 Steps

### Step 1: Install Dependencies
Open PowerShell or Command Prompt in the project directory and run:
```bash
npm install
```

### Step 2: Compile the Backend
Run the compilation script:
```bash
.\compile-backend.bat
```

Or manually compile with:
```bash
gcc -o backend\fs_backend.exe backend\main.c backend\filesystem.c backend\snapshot.c backend\versioning.c backend\btree.c backend\dedup.c backend\journal.c -lws2_32 -lm
```

### Step 3: Start the Application
Use the convenient start script:
```bash
.\start.bat
```

Or start manually:
```bash
# Terminal 1 - Backend
backend\fs_backend.exe

# Terminal 2 - Frontend
npm run dev
```

## 🎯 First Steps Tutorial

### 1. Create Your First File
1. Open http://localhost:3000 in your browser
2. Click on the **File Operations** tab
3. Click **Create File** button
4. Enter a name like "test.txt"
5. Click **Create**

### 2. Write Data to the File
1. Find your file in the list
2. Click the **Edit** button (pencil icon)
3. Choose a write strategy:
   - **Copy-on-Write (CoW)**: Best for versioning
   - **Redirect-on-Write (RoW)**: Best for performance
4. Enter some data in the text area
5. Click **Write**

### 3. Create a Snapshot
1. Go to the **Snapshots** tab
2. Click **Create Snapshot**
3. Enter a name like "Initial State"
4. Add a description (optional)
5. Click **Create**

### 4. View Block Allocation
1. Navigate to the **Block Manager** tab
2. See the visual grid of all blocks
3. Hover over blocks to see details
4. Use filters to view specific block types

### 5. Monitor Performance
1. Go to the **Performance** tab
2. View real-time charts and metrics
3. Check storage efficiency
4. Monitor operation timings

## 🧪 Try These Scenarios

### Scenario A: Test Copy-on-Write
1. Create a file and write data
2. Create a snapshot
3. Write new data to the same file using CoW
4. Notice how new blocks are allocated
5. Original data is preserved in the snapshot

### Scenario B: Test Deduplication
1. Create multiple files
2. Write identical content to them
3. Observe in Block Manager that blocks are shared
4. Check Performance tab for deduplication savings

### Scenario C: Version Management
1. Create a file
2. Write data multiple times
3. Go to **Versions** tab
4. Select the file to see version history
5. Add semantic tags to important versions
6. Try rolling back to a previous version

## 📊 Understanding the UI

### Overview Tab
- **System statistics** at the top
- **Block allocation map** - visual representation of all blocks
- **File system hierarchy** - tree view of files and snapshots

### Block Manager Tab
- **Block grid** - each square is a block
- **Color coding**:
  - Gray: Free blocks
  - Purple: Data blocks
  - Green: Inode blocks
  - Orange: Metadata blocks
  - Blue: Snapshot blocks
- **Numbers on blocks**: Reference count (shared blocks)
- **Icons**: CoW or Deduplicated indicators

### File Operations Tab
- **File list** with details
- **Create/Edit/Delete** operations
- **Strategy selection** for writes
- **Immutable policy** settings

### Snapshots Tab
- **Snapshot cards** with metadata
- **Hierarchical grouping** visualization
- **Semantic tags** for organization
- **Rollback** functionality

### Versions Tab
- **Timeline view** of file versions
- **Tag management** for versions
- **Rollback** to specific versions
- **Version comparison** (coming soon)

### Performance Tab
- **Real-time charts**:
  - Operations over time
  - Storage distribution
  - Operation statistics
- **Timing metrics** table
- **System health** indicators

## 🔧 Troubleshooting

### Backend Won't Start
**Problem**: "GCC not found" error
**Solution**: Install MinGW or TDM-GCC from https://jmeubank.github.io/tdm-gcc/

**Problem**: Port 8080 already in use
**Solution**: Close other applications using port 8080 or modify PORT in backend/main.c

### Frontend Won't Start
**Problem**: "Cannot find module" errors
**Solution**: Run `npm install` again

**Problem**: Port 3000 already in use
**Solution**: Modify port in vite.config.js

### Connection Issues
**Problem**: Frontend can't connect to backend
**Solution**: 
1. Ensure backend is running (check Terminal 1)
2. Verify backend is on http://localhost:8080
3. Check browser console for errors

## 💡 Tips & Tricks

1. **Auto-refresh**: The UI refreshes every 2 seconds automatically
2. **Block selection**: Click blocks in Block Manager to select and view details
3. **Hover tooltips**: Hover over blocks for instant information
4. **Keyboard shortcuts**: Use Tab to navigate between inputs in modals
5. **Performance**: For better performance, limit operations to <10,000 blocks

## 📝 Next Steps

- Experiment with different write strategies
- Create complex snapshot hierarchies
- Test rollback scenarios
- Monitor performance under different workloads
- Try the immutable zone policies
- Explore deduplication savings

## 🆘 Need Help?

- Check the full README.md for detailed documentation
- Review the code comments for implementation details
- Examine the browser console for debugging information
- Check the backend terminal for server logs

## 🎓 Learning Objectives

This simulator helps you understand:
- File system block allocation
- Copy-on-Write vs Redirect-on-Write strategies
- Snapshot and versioning mechanisms
- Data deduplication techniques
- Journaling for crash recovery
- Performance trade-offs in file systems

Enjoy exploring the file system simulator! 🚀
