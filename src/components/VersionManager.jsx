import React, { useState, useEffect } from 'react';
import { GitBranch, Tag, RotateCcw, Clock, FileText, Plus } from 'lucide-react';

const API_BASE = 'http://localhost:8080/api';

const VersionManager = ({ files, onRefresh }) => {
    const [selectedFile, setSelectedFile] = useState(null);
    const [versions, setVersions] = useState([]);
    const [showTagModal, setShowTagModal] = useState(false);
    const [selectedVersion, setSelectedVersion] = useState(null);
    const [tagName, setTagName] = useState('');
    const [tagDescription, setTagDescription] = useState('');

    // Fetch versions whenever a file is selected
    useEffect(() => {
        if (!selectedFile) return;
        fetchVersions(selectedFile.name);
    }, [selectedFile]);

    const fetchVersions = async (filename) => {
        try {
            const response = await fetch(`${API_BASE}/versions?name=${encodeURIComponent(filename)}`);
            const data = await response.json();
            if (data.success) {
                setVersions(data.versions);
            }
        } catch (err) {
            console.error('Failed to fetch versions:', err);
        }
    };

    // ===== CREATE VERSION =====
    const createVersion = async (file) => {
        try {
            const response = await fetch(`${API_BASE}/versions/create`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: file.name,
                    description: 'Manual version'
                })
            });
            const data = await response.json();
            if (data.success) {
                await onRefresh();           // Refresh file list
                await fetchVersions(file.name); // Refresh versions
            }
        } catch (err) {
            console.error('Failed to create version:', err);
        }
    };

    // ===== ROLLBACK VERSION =====
    const rollbackVersion = async (file, versionId) => {
        try {
            const response = await fetch(`${API_BASE}/versions/rollback`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: file.name, versionId })
            });
            const data = await response.json();
            if (data.success) {
                await onRefresh();
                await fetchVersions(file.name);
            }
        } catch (err) {
            console.error('Failed to rollback:', err);
        }
    };

    // ===== ADD TAG =====
    const addTag = async () => {
        if (!selectedFile || !selectedVersion) return;
        try {
            const response = await fetch(`${API_BASE}/versions/tag`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: selectedFile.name,
                    versionId: selectedVersion,
                    tag: tagName,
                    description: tagDescription
                })
            });
            const data = await response.json();
            if (data.success) {
                setTagName('');
                setTagDescription('');
                setShowTagModal(false);
                await fetchVersions(selectedFile.name);
            }
        } catch (err) {
            console.error('Failed to add tag:', err);
        }
    };

    const formatDate = (timestamp) => timestamp
        ? new Date(timestamp * 1000).toLocaleString()
        : '-';

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* File Selection */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <GitBranch size={20} /> Version Control
                    </h3>
                </div>
                <div className="card-body">
                    {files.length === 0 ? (
                        <div style={{ textAlign: 'center', padding: '3rem', color: 'var(--text-secondary)' }}>
                            <FileText size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                            <p>No files available. Create a file first!</p>
                        </div>
                    ) : (
                        <div className="grid grid-3" style={{ gap: '1rem' }}>
                            {files.map(file => (
                                <div
                                    key={file.id}
                                    onClick={() => setSelectedFile(file)}
                                    style={{
                                        padding: '1.5rem',
                                        background: selectedFile?.id === file.id ? 'var(--bg-hover)' : 'var(--bg-tertiary)',
                                        borderRadius: 'var(--radius-lg)',
                                        border: selectedFile?.id === file.id ? '2px solid var(--accent-primary)' : '1px solid var(--border-color)',
                                        cursor: 'pointer',
                                        transition: 'all 0.2s'
                                    }}
                                >
                                    <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', marginBottom: '0.5rem' }}>
                                        <FileText size={20} style={{ color: 'var(--accent-primary)' }} />
                                        <h4 style={{ fontSize: '1rem', fontWeight: '600' }}>{file.name}</h4>
                                    </div>
                                    <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                        <span>{file.size.toLocaleString()} bytes</span>
                                        <span className="badge badge-success">{file.versions} versions</span>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>

            {/* Version Timeline */}
            {selectedFile && (
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            <Clock size={20} /> Version History: {selectedFile.name}
                        </h3>
                        <button
                            className="btn btn-primary btn-sm"
                            onClick={() => createVersion(selectedFile)}
                        >
                            <Plus size={16} /> Create Version
                        </button>
                    </div>
                    <div className="card-body">
                        {versions.length === 0 ? (
                            <div style={{ textAlign: 'center', padding: '2rem', color: 'var(--text-secondary)' }}>
                                <GitBranch size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                                <p>No versions yet. Modify the file to create versions!</p>
                            </div>
                        ) : (
                            <div style={{ position: 'relative' }}>
                                <div style={{ position: 'absolute', left: '20px', top: '20px', bottom: '20px', width: '2px', background: 'var(--gradient-primary)', opacity: 0.3 }}></div>
                                {versions.slice().reverse().map((version, index) => (
                                    <div key={version.version_id} style={{ position: 'relative', paddingLeft: '60px', paddingBottom: '2rem' }}>
                                        <div style={{
                                            position: 'absolute',
                                            left: '12px',
                                            top: '8px',
                                            width: '18px',
                                            height: '18px',
                                            borderRadius: '50%',
                                            background: index === 0 ? 'var(--gradient-primary)' : 'var(--bg-tertiary)',
                                            border: '3px solid var(--bg-secondary)',
                                            boxShadow: index === 0 ? 'var(--shadow-glow)' : 'none'
                                        }}></div>
                                        <div style={{ padding: '1rem', background: 'var(--bg-tertiary)', borderRadius: 'var(--radius-md)', border: '1px solid var(--border-color)' }}>
                                            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: '0.5rem' }}>
                                                <div>
                                                    <h4 style={{ fontSize: '0.875rem', fontWeight: '600', color: 'var(--text-primary)' }}>
                                                        Version {version.version_id}
                                                        {index === 0 && <span className="badge badge-success" style={{ marginLeft: '0.5rem' }}>Current</span>}
                                                    </h4>
                                                    <div style={{ fontSize: '0.75rem', color: 'var(--text-tertiary)', marginTop: '0.25rem' }}>
                                                        Created: {formatDate(version.created_at)}
                                                    </div>
                                                </div>
                                                <div style={{ display: 'flex', gap: '0.5rem' }}>
                                                    <button className="btn btn-sm btn-secondary" onClick={() => { setSelectedVersion(version.version_id); setShowTagModal(true); }}>
                                                        <Tag size={14} />
                                                    </button>
                                                    {index !== 0 && (
                                                        <button className="btn btn-sm btn-secondary" onClick={() => rollbackVersion(selectedFile, version.version_id)}>
                                                            <RotateCcw size={14} />
                                                        </button>
                                                    )}
                                                </div>
                                            </div>
                                            <p style={{ fontSize: '0.75rem', color: 'var(--text-secondary)', marginBottom: '0.5rem' }}>
                                                {index === 0 ? 'Latest version' : `Version ${version.version_id} snapshot`}
                                            </p>
                                            {version.tag_count > 0 && (
                                                <div style={{ display: 'flex', gap: '0.5rem', flexWrap: 'wrap' }}>
                                                    {version.tags.map((t, i) => (
                                                        <span key={i} className="badge badge-primary">
                                                            <Tag size={10} /> {t.tag}
                                                        </span>
                                                    ))}
                                                </div>
                                            )}
                                        </div>
                                    </div>
                                ))}
                            </div>
                        )}
                    </div>
                </div>
            )}

            {/* Contextual Versioning Features */}
            <div className="card">
                <div className="card-header"><h3 className="card-title">Contextual Versioning Features</h3></div>
                <div className="card-body">
                    <div className="grid grid-2" style={{ gap: '1rem' }}>
                        <div style={{ padding: '1.5rem', background: 'var(--bg-tertiary)', borderRadius: 'var(--radius-md)', border: '2px solid var(--accent-primary)' }}>
                            <Tag size={24} style={{ color: 'var(--accent-primary)', marginBottom: '0.5rem' }} />
                            <h4 style={{ fontSize: '1rem', marginBottom: '0.5rem' }}>Semantic Tags</h4>
                            <p style={{ fontSize: '0.875rem', color: 'var(--text-secondary)' }}>
                                Tag versions with meaningful labels like "stable", "beta", "production" for easy identification and retrieval.
                            </p>
                        </div>
                        <div style={{ padding: '1.5rem', background: 'var(--bg-tertiary)', borderRadius: 'var(--radius-md)', border: '2px solid var(--accent-success)' }}>
                            <GitBranch size={24} style={{ color: 'var(--accent-success)', marginBottom: '0.5rem' }} />
                            <h4 style={{ fontSize: '1rem', marginBottom: '0.5rem' }}>Extended Attributes</h4>
                            <p style={{ fontSize: '0.875rem', color: 'var(--text-secondary)' }}>
                                Store custom metadata with each version including author, purpose, and other contextual information.
                            </p>
                        </div>
                    </div>
                </div>
            </div>

            {/* Add Tag Modal */}
            {showTagModal && selectedFile && selectedVersion && (
                <div className="modal-overlay" onClick={() => setShowTagModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Add Tag to Version {selectedVersion}</h3>
                            <button className="modal-close" onClick={() => setShowTagModal(false)}>×</button>
                        </div>
                        <div className="form-group">
                            <label className="form-label">Tag Name</label>
                            <input type="text" className="form-input" value={tagName} onChange={(e) => setTagName(e.target.value)} placeholder="e.g., stable, beta, production" autoFocus />
                        </div>
                        <div className="form-group">
                            <label className="form-label">Description</label>
                            <input type="text" className="form-input" value={tagDescription} onChange={(e) => setTagDescription(e.target.value)} placeholder="Describe this version..." />
                        </div>
                        <div style={{ display: 'flex', gap: '1rem', justifyContent: 'flex-end' }}>
                            <button className="btn btn-secondary" onClick={() => setShowTagModal(false)}>Cancel</button>
                            <button className="btn btn-primary" onClick={addTag} disabled={!tagName.trim()}>
                                <Tag size={16} /> Add Tag
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default VersionManager;