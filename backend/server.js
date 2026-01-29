const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const fs = require('./fs_core');

const app = express();
const PORT = 8080;

app.use(cors());
app.use(bodyParser.json());

// Helper for responses
const success = (res, data = {}) => res.json({ success: true, ...data });
const error = (res, msg) => res.status(400).json({ success: false, error: msg });

// --- Status ---
app.get('/api/status', (req, res) => {
    res.json(fs.getStats());
});

// --- Files ---
app.get('/api/files', (req, res) => {
    // Map inodes to expected format
    const files = fs.inodes.map(inode => ({
        id: inode.id,
        name: inode.name,
        size: inode.size,
        blocks: inode.blocks.length,
        versions: inode.versions.length,
        isDirectory: inode.isDir,
        immutablePolicy: 0
    }));
    success(res, { files });
});

app.post('/api/files', (req, res) => {
    const name = req.query.name || req.body.name;
    if (!name) return error(res, "Missing file name");
    try {
        fs.createFile(name);
        success(res, { message: "File created" });
    } catch (e) {
        error(res, e.message);
    }
});

app.post('/api/files/write', (req, res) => {
    const { name, data, strategy } = req.body;
    if (!name || data === undefined) return error(res, "Missing parameters");

    try {
        const inode = fs.writeFile(name, data, strategy || 'cow');
        success(res, {
            message: "Data written",
            inodeId: inode.id,
            size: inode.size
        });
    } catch (e) {
        error(res, e.message);
    }
});

// --- Blocks ---
app.get('/api/blocks', (req, res) => {
    const list = fs.blocks.map(b => ({
        id: b.id,
        type: b.type === 'FREE' ? 0 : (b.type === 'DATA' ? 1 : 2), // 0: Free, 1: Data
        refCount: b.refCount,
        isCow: b.refCount > 1, // Simplified view for frontend
        isDeduplicated: b.isDedup
    }));
    success(res, { blocks: list });
});

// --- Snapshots ---
app.get('/api/snapshots', (req, res) => {
    const list = fs.snapshots.map(s => ({
        id: s.id,
        name: s.name,
        totalSize: s.totalSize,
        inodeCount: s.inodes.length,
        tagCount: 0
    }));
    success(res, { snapshots: list });
});

app.post('/api/snapshots', (req, res) => {
    const { name, description } = req.body;
    if (!name) return error(res, "Missing name");
    try {
        const snap = fs.createSnapshot(name, description);
        success(res, { message: "Snapshot created", name: snap.name });
    } catch (e) {
        error(res, e.message);
    }
});

app.post('/api/snapshots/rollback', (req, res) => {
    const name = req.query.name || req.body.name;
    if (!name) return error(res, "Missing name");
    try {
        fs.rollbackSnapshot(name);
        success(res, { message: "Rollback successful" });
    } catch (e) {
        error(res, e.message);
    }
});

// --- Versions ---
app.post('/api/versions/create', (req, res) => {
    const { name, description } = req.body;
    try {
        const ver = fs.createVersion(name, description);
        success(res, { message: "Version created", versionId: ver.id });
    } catch (e) {
        error(res, e.message);
    }
});

app.post('/api/versions/rollback', (req, res) => {
    const { name, versionId } = req.body;
    try {
        fs.rollbackVersion(name, versionId);
        success(res, { message: "Rollback successful" });
    } catch (e) {
        error(res, e.message);
    }
});

app.get('/api/versions', (req, res) => {
    const name = req.query.name;
    const inode = fs.findInode(name);
    if (!inode) return error(res, "File not found");

    // Frontend expects snake_case for version_id and created_at
    const list = inode.versions.map(v => ({
        version_id: v.id,
        size: v.size,
        block_count: v.blocks.length,
        description: v.description,
        tags: v.tags || [],
        tag_count: (v.tags || []).length,
        created_at: v.created ? Math.floor(new Date(v.created).getTime() / 1000) : 0
    }));

    success(res, { versions: list });
});

app.post('/api/versions/tag', (req, res) => {
    const { name, versionId, tag, description } = req.body;
    try {
        fs.addVersionTag(name, versionId, tag, description);
        success(res, { message: "Tag added" });
    } catch (e) {
        error(res, e.message);
    }
});

app.listen(PORT, () => {
    console.log(`JS Backend running on http://localhost:${PORT}`);
});
