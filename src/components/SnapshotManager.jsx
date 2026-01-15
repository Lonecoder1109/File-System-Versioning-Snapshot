import React, { useState } from 'react';
import { Camera, Plus, RotateCcw, Tag } from 'lucide-react';

const API_BASE = 'http://localhost:8080/api';

const SnapshotManager = ({ snapshots, onRefresh }) => {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [showTagModal, setShowTagModal] = useState(false);
    const [selectedSnapshot, setSelectedSnapshot] = useState(null);
    const [snapshotName, setSnapshotName] = useState('');
    const [snapshotDescription, setSnapshotDescription] = useState('');
    const [tagName, setTagName] = useState('');
    const [tagDescription, setTagDescription] = useState('');

    // ✅ STATUS MESSAGE STATE
    const [statusMessage, setStatusMessage] = useState('');

    const showMessage = (msg) => {
        setStatusMessage(msg);
        setTimeout(() => setStatusMessage(''), 4000);
    };

    // ===== CREATE SNAPSHOT =====
    const createSnapshot = async () => {
        if (!snapshotName.trim()) return;
        try {
            const response = await fetch(`${API_BASE}/snapshots`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: snapshotName, description: snapshotDescription })
            });
            const data = await response.json();
            if (data.success) {
                setSnapshotName('');
                setSnapshotDescription('');
                setShowCreateModal(false);
                onRefresh();
                showMessage(`Snapshot "${data.name || snapshotName}" created successfully.`);
            } else {
                showMessage('Snapshot creation failed.');
            }
        } catch (err) {
            showMessage('Error creating snapshot.');
        }
    };

    // ===== ROLLBACK =====
    const rollbackSnapshot = async (snapshot) => {
        if (!snapshot) return;

        try {
            const response = await fetch(
                `${API_BASE}/snapshots/rollback?name=${encodeURIComponent(snapshot.name)}`,
                { method: 'POST' }
            );

            if (response.ok) {
                onRefresh();
                showMessage(
                    `Rollback to "${snapshot.name}" executed successfully.`
                );
            } else {
                showMessage('Rollback failed on server.');
            }
        } catch (err) {
            showMessage('Rollback request failed.');
            console.error(err);
        }
    };


    // ===== ADD TAG =====
    const addTag = async () => {
        if (!selectedSnapshot || !tagName.trim()) return;
        try {
            const response = await fetch(`${API_BASE}/snapshots/tag`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: selectedSnapshot.name,
                    tagName,
                    tagDescription
                })
            });
            const data = await response.json();
            if (data.success) {
                setTagName('');
                setTagDescription('');
                setShowTagModal(false);
                setSelectedSnapshot(null);
                onRefresh();
                showMessage(`Tag "${tagName}" added to snapshot.`);
            } else {
                showMessage('Failed to add tag.');
            }
        } catch (err) {
            showMessage('Error adding tag.');
        }
    };

    const formatSize = bytes => {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
        return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
    };

    return (
        <div className="grid" style={{ gap: '2rem' }}>

            {/* ✅ STATUS MESSAGE */}
            {statusMessage && (
                <div
                    style={{
                        padding: '0.75rem 1rem',
                        background: '#eef2ff',
                        border: '1px solid #c7d2fe',
                        borderRadius: '6px',
                        color: '#1e3a8a'
                    }}
                >
                    {statusMessage}
                </div>
            )}

            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Camera size={20}/> Snapshots ({snapshots.length})
                    </h3>
                    <button className="btn btn-primary btn-sm" onClick={() => setShowCreateModal(true)}>
                        <Plus size={16}/> Create Snapshot
                    </button>
                </div>

                <div className="card-body">
                    {snapshots.length === 0 ? (
                        <div style={{ textAlign:'center', padding:'3rem', color:'var(--text-secondary)' }}>
                            <Camera size={48} style={{ opacity:0.3, marginBottom:'1rem' }} />
                            <p>No snapshots yet. Create one!</p>
                        </div>
                    ) : (
                        <div className="grid grid-2" style={{ gap:'1rem' }}>
                            {snapshots.map(snapshot => (
                                <div key={snapshot.id}
                                     style={{ padding:'1rem', border:'1px solid var(--border-color)', borderRadius:'8px' }}>
                                    <h4>{snapshot.name}</h4>
                                    <div>Size: {formatSize(snapshot.totalSize)}</div>
                                    <div>Files: {snapshot.inodeCount}</div>
                                    <div>Tags: {snapshot.tagCount}</div>

                                    <div style={{ marginTop:'0.5rem', display:'flex', gap:'0.5rem' }}>
                                        <button
                                            className="btn btn-sm btn-secondary"
                                            onClick={() => rollbackSnapshot(snapshot)}
                                        >
                                            <RotateCcw size={14}/> Rollback
                                        </button>
                                        <button
                                            className="btn btn-sm btn-secondary"
                                            onClick={() => {
                                                setSelectedSnapshot(snapshot);
                                                setShowTagModal(true);
                                            }}
                                        >
                                            <Tag size={14}/> Tag
                                        </button>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>

            {/* Create Snapshot Modal */}
            {showCreateModal && (
                <div className="modal-overlay" onClick={() => setShowCreateModal(false)}>
                    <div className="modal" onClick={e => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Create Snapshot</h3>
                            <button className="modal-close" onClick={() => setShowCreateModal(false)}>×</button>
                        </div>
                        <div className="form-group">
                            <label>Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={snapshotName}
                                onChange={e => setSnapshotName(e.target.value)}
                                autoFocus
                            />
                        </div>
                        <div className="form-group">
                            <label>Description</label>
                            <textarea
                                className="form-textarea"
                                value={snapshotDescription}
                                onChange={e => setSnapshotDescription(e.target.value)}
                                rows={3}
                            />
                        </div>
                        <div style={{ display:'flex', justifyContent:'flex-end', gap:'1rem' }}>
                            <button className="btn btn-secondary" onClick={() => setShowCreateModal(false)}>Cancel</button>
                            <button className="btn btn-primary" onClick={createSnapshot} disabled={!snapshotName.trim()}>
                                Create
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {/* Add Tag Modal */}
            {showTagModal && selectedSnapshot && (
                <div className="modal-overlay" onClick={() => setShowTagModal(false)}>
                    <div className="modal" onClick={e => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Add Tag to {selectedSnapshot.name}</h3>
                            <button className="modal-close" onClick={() => setShowTagModal(false)}>×</button>
                        </div>
                        <div className="form-group">
                            <label>Tag Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={tagName}
                                onChange={e => setTagName(e.target.value)}
                                autoFocus
                            />
                        </div>
                        <div className="form-group">
                            <label>Description</label>
                            <input
                                type="text"
                                className="form-input"
                                value={tagDescription}
                                onChange={e => setTagDescription(e.target.value)}
                            />
                        </div>
                        <div style={{ display:'flex', justifyContent:'flex-end', gap:'1rem' }}>
                            <button className="btn btn-secondary" onClick={() => setShowTagModal(false)}>Cancel</button>
                            <button className="btn btn-primary" onClick={addTag} disabled={!tagName.trim()}>
                                Add Tag
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default SnapshotManager;
