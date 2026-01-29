# Comprehensive Testing Guide
## Advanced File System Simulator - End-to-End Testing

This guide provides step-by-step instructions to test **all features** of the File System Simulator.

---

## Prerequisites

1. **Backend Running**: `npm run start-backend` (Port 8080)
2. **Frontend Running**: `npm run dev` (Port 5173 or similar)
3. **Browser**: Open the frontend URL (usually `http://localhost:5173`)

---

## Test Suite 1: Basic File Operations

### Test 1.1: Create Files
**Objective**: Verify file creation functionality

**Steps**:
1. Navigate to **File Operations** tab
2. Click **Create File** button
3. Enter filename: `test1.txt`
4. Click **Create**
5. Verify file appears in the file list with:
   - ID (should be 1)
   - Name: `test1.txt`
   - Size: 0
   - Blocks: 0
   - Versions: 0

**Expected Result**: File created successfully, visible in table

**Repeat**: Create 2 more files (`test2.txt`, `test3.txt`)

---

### Test 1.2: Write Data (Copy-on-Write)
**Objective**: Test CoW write strategy

**Steps**:
1. Click **Edit** button on `test1.txt`
2. Select **Copy-on-Write (CoW)** from dropdown
3. Enter data: `Hello World - CoW Test`
4. Click **Write**
5. Click **Refresh Data** in header
6. Verify:
   - File size updated (should be ~22 bytes)
   - Blocks count = 1
   - In **Overview** tab → Block Allocation Map shows 1 used block
   - In **Performance** tab → CoW Writes = 1

**Expected Result**: Data written, block allocated, metrics updated

---

### Test 1.3: Write Data (Redirect-on-Write)
**Objective**: Test RoW write strategy

**Steps**:
1. Click **Edit** button on `test2.txt`
2. Select **Redirect-on-Write (RoW)** from dropdown
3. Enter data: `Hello World - RoW Test`
4. Click **Write**
5. Click **Refresh Data**
6. Verify:
   - File size updated
   - Blocks count = 1
   - In **Performance** tab → RoW Writes = 1
   - Block allocated at different position than CoW test

**Expected Result**: RoW write successful, different block allocation pattern

---

### Test 1.4: Overwrite Existing File (CoW)
**Objective**: Verify CoW behavior on existing data

**Steps**:
1. Click **Edit** on `test1.txt` (which already has data)
2. Select **Copy-on-Write (CoW)**
3. Enter NEW data: `Updated content using CoW strategy`
4. Click **Write**
5. Click **Refresh Data**
6. Go to **Block Manager** tab
7. Observe:
   - Old block should be freed
   - New block allocated (at higher index due to Next-Fit)
   - Total blocks used should still be ~3 (not doubled)

**Expected Result**: Old block freed, new block allocated sequentially

---

### Test 1.5: Overwrite Existing File (RoW)
**Objective**: Verify RoW behavior on existing data

**Steps**:
1. Click **Edit** on `test2.txt`
2. Select **Redirect-on-Write (RoW)**
3. Enter NEW data: `Updated content using RoW strategy`
4. Click **Write**
5. Click **Refresh Data**
6. Verify in **Block Manager**:
   - New block allocated
   - Old block freed
   - Block allocation continues sequentially

**Expected Result**: RoW redirects to new block location

---

## Test Suite 2: Deduplication

### Test 2.1: Write Duplicate Content
**Objective**: Test block-level deduplication

**Steps**:
1. Create new file: `duplicate1.txt`
2. Write data: `Duplicate content test`
3. Create another file: `duplicate2.txt`
4. Write **EXACT SAME** data: `Duplicate content test`
5. Click **Refresh Data**
6. Go to **Performance** tab
7. Check metrics:
   - Blocks Deduplicated should increase
   - Dedup Savings (MB) should show space saved

**Expected Result**: Both files share the same physical block (refCount = 2)

---

### Test 2.2: Verify Deduplication in Block Manager
**Steps**:
1. Go to **Block Manager** tab
2. Find the block used by both duplicate files
3. Verify:
   - `refCount` = 2
   - `isDeduplicated` = true
4. Go to **Overview** tab → Block Allocation Map
5. Hover over the deduplicated block
6. Verify tooltip shows `Refs: 2`

**Expected Result**: Single block shared by multiple files

---

## Test Suite 3: Snapshots

### Test 3.1: Create Snapshot
**Objective**: Capture file system state

**Steps**:
1. Ensure you have 3-4 files with data
2. Go to **Snapshots** tab
3. Click **Create Snapshot**
4. Enter name: `Snapshot_1`
5. Enter description: `Initial state with 4 files`
6. Click **Create**
7. Verify snapshot appears in list with:
   - Name
   - Total size
   - File count (inodeCount)

**Expected Result**: Snapshot created successfully

---

### Test 3.2: Modify Files After Snapshot
**Steps**:
1. Go to **File Operations** tab
2. Edit `test1.txt` and change content to: `Modified after snapshot`
3. Create a new file: `post_snapshot.txt` with data: `Created after snapshot`
4. Click **Refresh Data**
5. Verify current state is different from snapshot

**Expected Result**: File system state diverged from snapshot

---

### Test 3.3: Rollback to Snapshot
**Objective**: Restore previous state

**Steps**:
1. Go to **Snapshots** tab
2. Click **Rollback** button on `Snapshot_1`
3. Click **Refresh Data**
4. Go to **File Operations** tab
5. Verify:
   - `test1.txt` has ORIGINAL content (before modification)
   - `post_snapshot.txt` is GONE (doesn't exist)
   - File count matches snapshot state

**Expected Result**: File system restored to snapshot state

---

### Test 3.4: Create Multiple Snapshots
**Steps**:
1. Create snapshot: `Snapshot_2` (current state)
2. Modify files again
3. Create snapshot: `Snapshot_3`
4. Modify files again
5. Create snapshot: `Snapshot_4`
6. Go to **Snapshots** tab
7. Verify all snapshots listed
8. Try rolling back to `Snapshot_2`
9. Verify state matches `Snapshot_2`

**Expected Result**: Multiple snapshots work correctly

---

## Test Suite 4: Versioning

### Test 4.1: Create File Versions
**Objective**: Test version control

**Steps**:
1. Go to **Versions** tab
2. Select a file (e.g., `test1.txt`)
3. Click **Create Version** button
4. Verify version 1 created
5. Go to **File Operations** tab
6. Edit `test1.txt` with new content: `Version 2 content`
7. Go back to **Versions** tab
8. Click **Create Version** again
9. Verify version 2 appears in timeline

**Expected Result**: Multiple versions tracked

---

### Test 4.2: Rollback to Previous Version
**Steps**:
1. In **Versions** tab, select `test1.txt`
2. View version history (should show Version 1 and Version 2)
3. Click **Rollback** button on Version 1
4. Click **Refresh Data**
5. Go to **File Operations** tab
6. Verify `test1.txt` has content from Version 1

**Expected Result**: File content restored to Version 1

---

### Test 4.3: Add Tags to Versions
**Steps**:
1. Go to **Versions** tab
2. Select a file with versions
3. Click **Tag** button (tag icon) on a version
4. Enter tag: `stable`
5. Enter description: `Stable release version`
6. Click **Add Tag**
7. Verify tag appears on the version

**Expected Result**: Version tagged successfully

---

## Test Suite 5: Performance Monitoring

### Test 5.1: Monitor Block Utilization
**Steps**:
1. Go to **Performance** tab
2. Observe **Block Utilization** percentage
3. Create several large files (write long text)
4. Click **Refresh Data**
5. Verify Block Utilization increases

**Expected Result**: Real-time utilization tracking

---

### Test 5.2: Verify Write Patterns
**Steps**:
1. In **Performance** tab, locate **Write Patterns** card
2. Verify it shows:
   - CoW count
   - RoW count
3. Perform several CoW writes
4. Perform several RoW writes
5. Click **Refresh Data**
6. Verify counts update correctly

**Expected Result**: Accurate CoW/RoW tracking

---

### Test 5.3: Check Performance Timings
**Steps**:
1. In **Performance** tab, scroll to **Performance Timings** table
2. Verify it shows:
   - Write Operations (avg time in ms)
   - Snapshot Creation (avg time)
   - Rollback Operations (avg time)
3. Perform multiple operations
4. Refresh and verify timings update

**Expected Result**: Timing metrics calculated

---

## Test Suite 6: Block Visualization

### Test 6.1: Block Allocation Map
**Steps**:
1. Go to **Overview** tab
2. Observe **Block Allocation Map**
3. Verify:
   - Free blocks (gray/transparent)
   - Used blocks (colored)
4. Hover over blocks to see details
5. Create new files and refresh
6. Verify new blocks appear

**Expected Result**: Visual representation of block usage

---

### Test 6.2: Hierarchy View
**Steps**:
1. In **Overview** tab, click **Hierarchy** button
2. Verify file tree structure shows:
   - Root directory
   - All files with sizes and block counts
   - Snapshots section (if any)
3. Switch back to **Block View**

**Expected Result**: Both views display correctly

---

## Test Suite 7: Edge Cases & Stress Tests

### Test 7.1: Empty File Operations
**Steps**:
1. Create file: `empty.txt`
2. Write empty string (just click Write without entering data)
3. Verify file size = 0, blocks = 0

**Expected Result**: Empty files handled gracefully

---

### Test 7.2: Large File Write
**Steps**:
1. Create file: `large.txt`
2. Write data larger than 4KB (e.g., paste 10KB of text)
3. Verify:
   - Multiple blocks allocated (size / 4096)
   - Block count reflects actual usage

**Expected Result**: Multi-block files work correctly

---

### Test 7.3: Rapid Operations
**Steps**:
1. Quickly create 5 files
2. Quickly write to all 5 files
3. Create snapshot
4. Modify all files
5. Rollback
6. Click **Refresh Data**
7. Verify system state is consistent

**Expected Result**: No race conditions or data corruption

---

### Test 7.4: Snapshot of Empty System
**Steps**:
1. Rollback to a snapshot (or refresh page to reset)
2. Delete/rollback until no files exist
3. Create snapshot: `Empty_State`
4. Create files
5. Rollback to `Empty_State`
6. Verify all files removed

**Expected Result**: Empty snapshots work

---

## Test Suite 8: Integration Tests

### Test 8.1: Full Workflow
**Complete end-to-end scenario**:

1. **Setup**:
   - Create 3 files: `doc1.txt`, `doc2.txt`, `doc3.txt`
   - Write data to each using different strategies (CoW, RoW, CoW)

2. **Snapshot**:
   - Create snapshot: `Baseline`

3. **Modifications**:
   - Edit `doc1.txt` (CoW)
   - Create version of `doc1.txt`
   - Edit `doc2.txt` (RoW)
   - Create new file: `doc4.txt`

4. **Second Snapshot**:
   - Create snapshot: `Modified_State`

5. **More Changes**:
   - Delete content from `doc3.txt` (write empty)
   - Tag version of `doc1.txt` as `production`

6. **Rollback Test**:
   - Rollback to `Baseline`
   - Verify `doc4.txt` is gone
   - Verify `doc1.txt`, `doc2.txt`, `doc3.txt` have original content

7. **Forward Rollback**:
   - Rollback to `Modified_State`
   - Verify `doc4.txt` is back
   - Verify modifications restored

**Expected Result**: Complete workflow executes without errors

---

## Test Suite 9: Metrics Validation

### Test 9.1: Cross-Tab Consistency
**Steps**:
1. Note metrics in **Performance** tab:
   - Total Writes
   - CoW Writes
   - RoW Writes
   - Blocks Allocated
   - Blocks Freed

2. Go to **Overview** tab header stats
3. Verify "Total Writes" matches

4. Go to **Block Manager**
5. Count used blocks manually
6. Verify matches "Used Blocks" in Overview

**Expected Result**: All metrics consistent across tabs

---

### Test 9.2: Deduplication Savings Calculation
**Steps**:
1. Create 5 files with identical content (100 bytes each)
2. Click **Refresh Data**
3. Go to **Performance** tab
4. Verify:
   - Dedup Savings shows ~400 bytes saved (4 files sharing 1 block)
   - Blocks Deduplicated = 1 (or more if multiple dedup blocks)

**Expected Result**: Savings calculated correctly

---

## Test Suite 10: UI/UX Testing

### Test 10.1: Responsive Design
**Steps**:
1. Resize browser window (narrow, wide, medium)
2. Verify all tabs remain functional
3. Check mobile view (if applicable)

**Expected Result**: UI adapts to different screen sizes

---

### Test 10.2: Error Handling
**Steps**:
1. Try creating file with empty name → Should show error
2. Try creating duplicate filename → Should show error
3. Try writing to non-existent file → Should handle gracefully
4. Try rolling back to non-existent snapshot → Should error gracefully

**Expected Result**: User-friendly error messages

---

## Test Checklist Summary

Use this checklist to track your testing progress:

- [ ] **File Operations**
  - [ ] Create files
  - [ ] Write with CoW
  - [ ] Write with RoW
  - [ ] Overwrite files
  - [ ] Empty files
  - [ ] Large files (multi-block)

- [ ] **Deduplication**
  - [ ] Duplicate content detection
  - [ ] Shared block verification
  - [ ] Savings calculation

- [ ] **Snapshots**
  - [ ] Create snapshot
  - [ ] Rollback to snapshot
  - [ ] Multiple snapshots
  - [ ] Empty system snapshot

- [ ] **Versioning**
  - [ ] Create versions
  - [ ] Rollback versions
  - [ ] Tag versions
  - [ ] Version timeline

- [ ] **Performance**
  - [ ] Block utilization tracking
  - [ ] Write patterns (CoW/RoW)
  - [ ] Timing metrics
  - [ ] Dedup savings

- [ ] **Visualization**
  - [ ] Block allocation map
  - [ ] Hierarchy view
  - [ ] Block hover tooltips
  - [ ] Color coding

- [ ] **Integration**
  - [ ] Full workflow test
  - [ ] Cross-tab consistency
  - [ ] Rapid operations
  - [ ] State persistence

- [ ] **Edge Cases**
  - [ ] Error handling
  - [ ] Empty states
  - [ ] Maximum capacity
  - [ ] Concurrent operations

---

## Expected Final State

After completing all tests, your system should demonstrate:

1. ✅ **Functional file operations** (create, write, read)
2. ✅ **Working CoW and RoW strategies** with visible differences
3. ✅ **Deduplication** reducing storage usage
4. ✅ **Snapshots** preserving and restoring state
5. ✅ **Versioning** tracking file history
6. ✅ **Performance metrics** accurately reflecting operations
7. ✅ **Visual representations** showing block usage
8. ✅ **Consistent state** across all tabs and views

---

## Troubleshooting

**Issue**: Changes not appearing
- **Solution**: Click **Refresh Data** button in header

**Issue**: Metrics seem incorrect
- **Solution**: Restart backend (`npm run start-backend`)

**Issue**: Blocks not showing in visualization
- **Solution**: Ensure files have data written (size > 0)

**Issue**: Deduplication not working
- **Solution**: Ensure EXACT same content (case-sensitive, no extra spaces)

---

## Performance Benchmarks

After testing, you should observe:
- **Write Time**: < 1ms per operation
- **Snapshot Time**: < 100ms
- **Rollback Time**: < 200ms
- **Deduplication**: Immediate (same write operation)
- **Block Allocation**: Next-Fit pattern (sequential)

---

**Testing Complete!** 🎉

You have now tested all major features of the Advanced File System Simulator.
