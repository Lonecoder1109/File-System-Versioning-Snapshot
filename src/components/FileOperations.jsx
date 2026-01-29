import React, { useState } from 'react';
import { FileText, Plus, Edit } from 'lucide-react';

const API_BASE = 'http://localhost:8080/api';

const FileOperations = ({ files, onRefresh }) => {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [showWriteModal, setShowWriteModal] = useState(false);
    const [selectedFile, setSelectedFile] = useState(null);
    const [newFileName, setNewFileName] = useState('');
    const [writeData, setWriteData] = useState('');
    const [writeStrategy, setWriteStrategy] = useState('cow'); // Only CoW now
    const [localFiles, setLocalFiles] = useState(files);

    // Sync prop files with local state
    React.useEffect(() => {
        setLocalFiles(files);
    }, [files]);

    // ===== CREATE FILE =====
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
            } else {
                console.error('Create file failed:', data.error);
            }
        } catch (err) {
            console.error('Failed to create file:', err);
        }
    };

    // ===== WRITE FILE =====
    const writeFile = async () => {
        if (!selectedFile || !writeData.trim()) return;

        try {
            const response = await fetch(`${API_BASE}/files/write`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: selectedFile.name,
                    strategy: writeStrategy.toLowerCase(), // Always 'cow'
                    data: writeData
                })
            });
            const data = await response.json();

            if (data.success) {
                await onRefresh();   // <-- refresh file list from backend

                setWriteData('');
                setShowWriteModal(false);
                setSelectedFile(null);
            } else {
                console.error('Write failed:', data.error);
            }
        } catch (err) {
            console.error('Failed to write file:', err);
        }
    };

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* File List */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <FileText size={20} /> File System
                    </h3>
                    <button className="btn btn-primary btn-sm" onClick={() => setShowCreateModal(true)}>
                        <Plus size={16} /> Create File
                    </button>
                </div>
                <div className="card-body">
                    {localFiles.length === 0 ? (
                        <div style={{ textAlign: 'center', padding: '3rem', color: 'var(--text-secondary)' }}>
                            <FileText size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                            <p>No files yet. Create your first file!</p>
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
                                        <th>Actions</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    {localFiles.map(file => (
                                        <tr key={file.id}>
                                            <td style={{ fontFamily: 'var(--font-mono)' }}>{file.id}</td>
                                            <td>
                                                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                                                    <FileText size={16} /> {file.name}
                                                </div>
                                            </td>
                                            <td>{file.size}</td>
                                            <td>
                                                <span className="badge badge-primary">
                                                    {file.blocks}
                                                </span>
                                            </td>

                                            {/* FIXED VERSION COUNT */}
                                            <td>
                                                <span className="badge badge-success">
                                                    {file.versionCount ?? file.versions}
                                                </span>
                                            </td>

                                            <td>
                                                <button
                                                    className="btn btn-sm btn-secondary"
                                                    onClick={() => {
                                                        setSelectedFile(file);
                                                        setShowWriteModal(true);
                                                    }}
                                                >
                                                    <Edit size={14} />
                                                </button>
                                            </td>
                                        </tr>
                                    ))}
                                </tbody>
                            </table>
                        </div>
                    )}
                </div>
            </div>

            {/* Create File Modal */}
            {showCreateModal && (
                <div className="modal-overlay" onClick={() => setShowCreateModal(false)}>
                    <div className="modal" onClick={e => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Create New File</h3>
                            <button className="modal-close" onClick={() => setShowCreateModal(false)}>×</button>
                        </div>
                        <div className="form-group">
                            <label>File Name</label>
                            <input
                                type="text"
                                className="form-input"
                                value={newFileName}
                                onChange={e => setNewFileName(e.target.value)}
                                placeholder="Enter file name..."
                                autoFocus
                            />
                        </div>
                        <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '1rem' }}>
                            <button className="btn btn-secondary" onClick={() => setShowCreateModal(false)}>Cancel</button>
                            <button className="btn btn-primary" onClick={createFile} disabled={!newFileName.trim()}>
                                Create
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {/* Write File Modal */}
            {showWriteModal && selectedFile && (
                <div className="modal-overlay" onClick={() => setShowWriteModal(false)}>
                    <div className="modal" onClick={e => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3 className="modal-title">Write to {selectedFile.name}</h3>
                            <button className="modal-close" onClick={() => setShowWriteModal(false)}>×</button>
                        </div>
                        <div className="form-group">
                            <label>Write Strategy</label>
                            <select
                                className="form-select"
                                value={writeStrategy}
                                onChange={e => setWriteStrategy(e.target.value)}
                            >
                                <option value="cow">Copy-on-Write (CoW)</option>
                                <option value="row">Redirect-on-Write (RoW)</option>
                            </select>
                        </div>
                        <div className="form-group">
                            <label>Data</label>
                            <textarea
                                className="form-textarea"
                                value={writeData}
                                onChange={e => setWriteData(e.target.value)}
                                rows={6}
                                autoFocus
                            />
                        </div>
                        <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '1rem' }}>
                            <button className="btn btn-secondary" onClick={() => setShowWriteModal(false)}>Cancel</button>
                            <button className="btn btn-primary" onClick={writeFile} disabled={!writeData.trim()}>
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
