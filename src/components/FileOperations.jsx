import React, { useState } from 'react';
import { FileText, Plus, Edit, Trash2, Save, Upload, Download } from 'lucide-react';

const API_BASE = 'http://localhost:8080/api';

const FileOperations = ({ files, onRefresh }) => {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [showWriteModal, setShowWriteModal] = useState(false);
    const [selectedFile, setSelectedFile] = useState(null);
    const [newFileName, setNewFileName] = useState('');
    const [writeData, setWriteData] = useState('');
    const [writeStrategy, setWriteStrategy] = useState('cow');
    const [immutablePolicy, setImmutablePolicy] = useState('none');

    const createFile = async () => {
        if (!newFileName.trim()) return;

        try {
            const response = await fetch(`${API_BASE}/files?name=${encodeURIComponent(newFileName)}`, {
                method: 'POST'
            });
            const data = await response.json();

            if (data.success) {
                setNewFileName('');
                setShowCreateModal(false);
                onRefresh();
            }
        } catch (error) {
            console.error('Failed to create file:', error);
        }
    };

    const writeFile = async () => {
        if (!selectedFile || !writeData.trim()) return;

        try {
            const response = await fetch(
                `${API_BASE}/files/write?id=${selectedFile.id}&strategy=${writeStrategy}`,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'text/plain' },
                    body: writeData
                }
            );
            const data = await response.json();

            if (data.success) {
                setWriteData('');
                setShowWriteModal(false);
                setSelectedFile(null);
                onRefresh();
            }
        } catch (error) {
            console.error('Failed to write file:', error);
        }
    };

    const policyOptions = [
        { value: 'none', label: 'None', description: 'No restrictions' },
        { value: 'readonly', label: 'Read Only', description: 'Cannot be modified' },
        { value: 'appendonly', label: 'Append Only', description: 'Can only append data' },
        { value: 'worm', label: 'WORM', description: 'Write Once Read Many' }
    ];

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* File List */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <FileText size={20} />
                        File System
                    </h3>
                    <button
                        className="btn btn-primary btn-sm"
                        onClick={() => setShowCreateModal(true)}
                    >
                        <Plus size={16} />
                        Create File
                    </button>
                </div>
                <div className="card-body">
                    {files.length === 0 ? (
                        <div style={{
                            textAlign: 'center',
                            padding: '3rem',
                            color: 'var(--text-secondary)'
                        }}>
                            <FileText size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                            <p>No files yet. Create your first file to get started!</p>
                        </div>
                    ) : (
                        <div className="table-container">
                            <table className="table">
                                <thead>
                                    <tr>
                                        <th>ID</th>
                                        <th>Name</th>
                                        <th>Size</th>
                                        <th>Blocks</th>
                                        <th>Versions</th>
                                        <th>Policy</th>
                                        <th>Actions</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    {files.map(file => (
                                        <tr key={file.id}>
                                            <td style={{ fontFamily: 'var(--font-mono)' }}>{file.id}</td>
                                            <td>
                                                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                                                    <FileText size={16} />
                                                    {file.name}
                                                </div>
                                            </td>
                                            <td>{file.size.toLocaleString()} bytes</td>
                                            <td>
                                                <span className="badge badge-primary">{file.blocks}</span>
                                            </td>
                                            <td>
                                                <span className="badge badge-success">{file.versions}</span>
                                            </td>
                                            <td>
                                                {file.immutablePolicy === 0 ? (
                                                    <span className="badge badge-secondary">None</span>
                                                ) : file.immutablePolicy === 1 ? (
                                                    <span className="badge badge-warning">Read Only</span>
                                                ) : file.immutablePolicy === 2 ? (
                                                    <span className="badge badge-info">Append Only</span>
                                                ) : (
                                                    <span className="badge badge-danger">WORM</span>
                                                )}
                                            </td>
                                            <td>
                                                <div style={{ display: 'flex', gap: '0.5rem' }}>
                                                    <button
                                                        className="btn btn-sm btn-secondary"
                                                        onClick={() => {
                                                            setSelectedFile(file);
                                                            setShowWriteModal(true);
                                                        }}
                                                    >
                                                        <Edit size={14} />
                                                    </button>
                                                </div>
                                            </td>
                                        </tr>
                                    ))}
                                </tbody>
                            </table>
                        </div>
                    )}
                </div>
            </div>

            {/* File Operations Guide */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">Write Strategies</h3>
                </div>
                <div className="card-body">
                    <div className="grid grid-2" style={{ gap: '1rem' }}>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            border: '2px solid var(--accent-primary)'
                        }}>
                            <h4 style={{
                                fontSize: '1rem',
                                marginBottom: '0.5rem',
                                color: 'var(--accent-primary)'
                            }}>
                                Copy-on-Write (CoW)
                            </h4>
                            <p style={{ fontSize: '0.875rem', color: 'var(--text-secondary)' }}>
                                Creates a copy of blocks before modification. Preserves original data and enables
                                efficient snapshots. Ideal for versioning and rollback scenarios.
                            </p>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            border: '2px solid var(--accent-success)'
                        }}>
                            <h4 style={{
                                fontSize: '1rem',
                                marginBottom: '0.5rem',
                                color: 'var(--accent-success)'
                            }}>
                                Redirect-on-Write (RoW)
                            </h4>
                            <p style={{ fontSize: '0.875rem', color: 'var(--text-secondary)' }}>
                                Allocates new blocks for writes and updates pointers. More efficient for large
                                sequential writes. Better performance for write-heavy workloads.
                            </p>
                        </div>
                    </div>
                </div>
            </div>

            {/* Create File Modal */}
            {showCreateModal && (
                <div className="modal-overlay" onClick={() => setShowCreateModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Create New File</h3>
                            <button className="modal-close" onClick={() => setShowCreateModal(false)}>
                                ×
                            </button>
                        </div>
                        <div className="form-group">
                            <label className="form-label">File Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={newFileName}
                                onChange={(e) => setNewFileName(e.target.value)}
                                placeholder="Enter file name..."
                                autoFocus
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
                                onClick={createFile}
                                disabled={!newFileName.trim()}
                            >
                                <Plus size={16} />
                                Create
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {/* Write File Modal */}
            {showWriteModal && selectedFile && (
                <div className="modal-overlay" onClick={() => setShowWriteModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Write to {selectedFile.name}</h3>
                            <button className="modal-close" onClick={() => setShowWriteModal(false)}>
                                ×
                            </button>
                        </div>
                        <div className="form-group">
                            <label className="form-label">Write Strategy</label>
                            <select
                                className="form-select"
                                value={writeStrategy}
                                onChange={(e) => setWriteStrategy(e.target.value)}
                            >
                                <option value="cow">Copy-on-Write (CoW)</option>
                                <option value="row">Redirect-on-Write (RoW)</option>
                            </select>
                        </div>
                        <div className="form-group">
                            <label className="form-label">Data</label>
                            <textarea
                                className="form-textarea"
                                value={writeData}
                                onChange={(e) => setWriteData(e.target.value)}
                                placeholder="Enter data to write..."
                                rows={6}
                                autoFocus
                            />
                        </div>
                        <div style={{ display: 'flex', gap: '1rem', justifyContent: 'flex-end' }}>
                            <button
                                className="btn btn-secondary"
                                onClick={() => setShowWriteModal(false)}
                            >
                                Cancel
                            </button>
                            <button
                                className="btn btn-primary"
                                onClick={writeFile}
                                disabled={!writeData.trim()}
                            >
                                <Save size={16} />
                                Write
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default FileOperations;
