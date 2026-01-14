import React, { useState } from 'react';
import { Camera, Plus, Trash2, RotateCcw, Tag, Layers, GitBranch } from 'lucide-react';

const API_BASE = '/api';

const SnapshotManager = ({ snapshots, onRefresh }) => {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [showTagModal, setShowTagModal] = useState(false);
    const [selectedSnapshot, setSelectedSnapshot] = useState(null);
    const [snapshotName, setSnapshotName] = useState('');
    const [snapshotDescription, setSnapshotDescription] = useState('');
    const [tagName, setTagName] = useState('');
    const [tagDescription, setTagDescription] = useState('');

    const createSnapshot = async () => {
        if (!snapshotName.trim()) return;

        try {
            const response = await fetch(
                `${API_BASE}/snapshots?name=${encodeURIComponent(snapshotName)}&description=${encodeURIComponent(snapshotDescription)}`,
                { method: 'POST' }
            );
            const data = await response.json();

            if (data.success) {
                setSnapshotName('');
                setSnapshotDescription('');
                setShowCreateModal(false);
                onRefresh();
            }
        } catch (error) {
            console.error('Failed to create snapshot:', error);
        }
    };

    const formatDate = (timestamp) => {
        return new Date(timestamp * 1000).toLocaleString();
    };

    const formatSize = (bytes) => {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
        return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
    };

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* Snapshot List */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Camera size={20} />
                        Snapshots ({snapshots.length})
                    </h3>
                    <button
                        className="btn btn-primary btn-sm"
                        onClick={() => setShowCreateModal(true)}
                    >
                        <Plus size={16} />
                        Create Snapshot
                    </button>
                </div>
                <div className="card-body">
                    {snapshots.length === 0 ? (
                        <div style={{
                            textAlign: 'center',
                            padding: '3rem',
                            color: 'var(--text-secondary)'
                        }}>
                            <Camera size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                            <p>No snapshots yet. Create a snapshot to preserve the current state!</p>
                        </div>
                    ) : (
                        <div className="grid grid-2" style={{ gap: '1rem' }}>
                            {snapshots.map(snapshot => (
                                <div
                                    key={snapshot.id}
                                    style={{
                                        padding: '1.5rem',
                                        background: 'var(--bg-tertiary)',
                                        borderRadius: 'var(--radius-lg)',
                                        border: '1px solid var(--border-color)',
                                        transition: 'all 0.2s'
                                    }}
                                    onMouseEnter={(e) => {
                                        e.currentTarget.style.borderColor = 'var(--accent-primary)';
                                        e.currentTarget.style.transform = 'translateY(-2px)';
                                    }}
                                    onMouseLeave={(e) => {
                                        e.currentTarget.style.borderColor = 'var(--border-color)';
                                        e.currentTarget.style.transform = 'translateY(0)';
                                    }}
                                >
                                    <div style={{
                                        display: 'flex',
                                        justifyContent: 'space-between',
                                        alignItems: 'flex-start',
                                        marginBottom: '1rem'
                                    }}>
                                        <div>
                                            <h4 style={{
                                                fontSize: '1.125rem',
                                                fontWeight: '600',
                                                color: 'var(--text-primary)',
                                                marginBottom: '0.25rem'
                                            }}>
                                                {snapshot.name}
                                            </h4>
                                            <div style={{
                                                fontSize: '0.75rem',
                                                color: 'var(--text-tertiary)',
                                                fontFamily: 'var(--font-mono)'
                                            }}>
                                                ID: {snapshot.id}
                                            </div>
                                        </div>
                                        <span className="badge badge-primary">
                                            <Camera size={12} />
                                            {snapshot.inodeCount} files
                                        </span>
                                    </div>

                                    {snapshot.description && (
                                        <p style={{
                                            fontSize: '0.875rem',
                                            color: 'var(--text-secondary)',
                                            marginBottom: '1rem'
                                        }}>
                                            {snapshot.description}
                                        </p>
                                    )}

                                    <div style={{
                                        display: 'grid',
                                        gridTemplateColumns: '1fr 1fr',
                                        gap: '0.5rem',
                                        marginBottom: '1rem',
                                        fontSize: '0.75rem'
                                    }}>
                                        <div>
                                            <span style={{ color: 'var(--text-tertiary)' }}>Size:</span>
                                            <span style={{
                                                color: 'var(--text-primary)',
                                                marginLeft: '0.5rem',
                                                fontFamily: 'var(--font-mono)'
                                            }}>
                                                {formatSize(snapshot.size)}
                                            </span>
                                        </div>
                                        <div>
                                            <span style={{ color: 'var(--text-tertiary)' }}>Tags:</span>
                                            <span style={{
                                                color: 'var(--text-primary)',
                                                marginLeft: '0.5rem'
                                            }}>
                                                {snapshot.tagCount}
                                            </span>
                                        </div>
                                    </div>

                                    {snapshot.groupName && (
                                        <div style={{ marginBottom: '1rem' }}>
                                            <span className="badge badge-success">
                                                <Layers size={12} />
                                                {snapshot.groupName}
                                            </span>
                                        </div>
                                    )}

                                    {snapshot.parentSnapshot > 0 && (
                                        <div style={{
                                            fontSize: '0.75rem',
                                            color: 'var(--text-secondary)',
                                            marginBottom: '1rem'
                                        }}>
                                            <GitBranch size={12} style={{ display: 'inline', marginRight: '0.25rem' }} />
                                            Parent: Snapshot #{snapshot.parentSnapshot}
                                            {snapshot.childCount > 0 && ` | ${snapshot.childCount} children`}
                                        </div>
                                    )}

                                    <div style={{
                                        display: 'flex',
                                        gap: '0.5rem',
                                        paddingTop: '1rem',
                                        borderTop: '1px solid var(--border-color)'
                                    }}>
                                        <button className="btn btn-sm btn-secondary" style={{ flex: 1 }}>
                                            <RotateCcw size={14} />
                                            Rollback
                                        </button>
                                        <button
                                            className="btn btn-sm btn-secondary"
                                            onClick={() => {
                                                setSelectedSnapshot(snapshot);
                                                setShowTagModal(true);
                                            }}
                                        >
                                            <Tag size={14} />
                                        </button>
                                        <button className="btn btn-sm btn-danger">
                                            <Trash2 size={14} />
                                        </button>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>

            {/* Snapshot Features */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">Advanced Snapshot Features</h3>
                </div>
                <div className="card-body">
                    <div className="grid grid-3" style={{ gap: '1rem' }}>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <Layers size={32} style={{
                                color: 'var(--accent-primary)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Hierarchical Grouping
                            </h4>
                            <p style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                Organize snapshots in parent-child relationships
                            </p>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <Tag size={32} style={{
                                color: 'var(--accent-success)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Semantic Tagging
                            </h4>
                            <p style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                Add contextual tags for easy retrieval
                            </p>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <Camera size={32} style={{
                                color: 'var(--accent-warning)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Adaptive Granularity
                            </h4>
                            <p style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                Dynamic snapshot scheduling based on activity
                            </p>
                        </div>
                    </div>
                </div>
            </div>

            {/* Create Snapshot Modal */}
            {showCreateModal && (
                <div className="modal-overlay" onClick={() => setShowCreateModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Create Snapshot</h3>
                            <button className="modal-close" onClick={() => setShowCreateModal(false)}>
                                ×
                            </button>
                        </div>
                        <div className="form-group">
                            <label className="form-label">Snapshot Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={snapshotName}
                                onChange={(e) => setSnapshotName(e.target.value)}
                                placeholder="e.g., Before Major Update"
                                autoFocus
                            />
                        </div>
                        <div className="form-group">
                            <label className="form-label">Description (Optional)</label>
                            <textarea
                                className="form-textarea"
                                value={snapshotDescription}
                                onChange={(e) => setSnapshotDescription(e.target.value)}
                                placeholder="Describe what this snapshot captures..."
                                rows={3}
                            />
                        </div>
                        <div style={{ display: 'flex', gap: '1rem', justifyContent: 'flex-end' }}>
                            <button
                                className="btn btn-secondary"
                                onClick={() => setShowCreateModal(false)}
                            >
                                Cancel
                            </button>
                            <button
                                className="btn btn-primary"
                                onClick={createSnapshot}
                                disabled={!snapshotName.trim()}
                            >
                                <Camera size={16} />
                                Create
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {/* Add Tag Modal */}
            {showTagModal && selectedSnapshot && (
                <div className="modal-overlay" onClick={() => setShowTagModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Add Tag to {selectedSnapshot.name}</h3>
                            <button className="modal-close" onClick={() => setShowTagModal(false)}>
                                ×
                            </button>
                        </div>
                        <div className="form-group">
                            <label className="form-label">Tag Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={tagName}
                                onChange={(e) => setTagName(e.target.value)}
                                placeholder="e.g., production, backup, milestone"
                                autoFocus
                            />
                        </div>
                        <div className="form-group">
                            <label className="form-label">Description</label>
                            <input
                                type="text"
                                className="form-input"
                                value={tagDescription}
                                onChange={(e) => setTagDescription(e.target.value)}
                                placeholder="Describe this tag..."
                            />
                        </div>
                        <div style={{ display: 'flex', gap: '1rem', justifyContent: 'flex-end' }}>
                            <button
                                className="btn btn-secondary"
                                onClick={() => setShowTagModal(false)}
                            >
                                Cancel
                            </button>
                            <button
                                className="btn btn-primary"
                                onClick={() => {
                                    // Add tag logic here
                                    setShowTagModal(false);
                                }}
                                disabled={!tagName.trim()}
                            >
                                <Tag size={16} />
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
