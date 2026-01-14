import React, { useState } from 'react';
import { HardDrive, Layers, Zap, Copy, Database, TrendingUp } from 'lucide-react';

const FileSystemVisualizer = ({ blocks, files, snapshots, systemStatus }) => {
    const [viewMode, setViewMode] = useState('blocks');

    if (!systemStatus) {
        return (
            <div className="card">
                <div className="card-body" style={{ textAlign: 'center', padding: '3rem' }}>
                    <div className="spinner"></div>
                    <p style={{ color: 'var(--text-secondary)', marginTop: '1rem' }}>
                        Loading file system...
                    </p>
                </div>
            </div>
        );
    }

    const blockTypes = {
        0: { name: 'FREE', color: '#64748b', bg: 'rgba(100, 116, 139, 0.1)' },
        1: { name: 'DATA', color: '#667eea', bg: 'rgba(102, 126, 234, 0.2)' },
        2: { name: 'INODE', color: '#10b981', bg: 'rgba(16, 185, 129, 0.2)' },
        3: { name: 'METADATA', color: '#f59e0b', bg: 'rgba(245, 158, 11, 0.2)' },
        4: { name: 'SNAPSHOT', color: '#3b82f6', bg: 'rgba(59, 130, 246, 0.2)' },
        5: { name: 'JOURNAL', color: '#f59e0b', bg: 'rgba(245, 158, 11, 0.2)' },
        6: { name: 'BTREE', color: '#8b5cf6', bg: 'rgba(139, 92, 246, 0.2)' },
        7: { name: 'DEDUP', color: '#ec4899', bg: 'rgba(236, 72, 153, 0.2)' }
    };

    // Calculate statistics
    const totalSpace = systemStatus.totalBlocks * 4096; // 4KB per block
    const usedSpace = systemStatus.usedBlocks * 4096;
    const freeSpace = totalSpace - usedSpace;

    const blockTypeStats = blocks.reduce((acc, block) => {
        const typeName = blockTypes[block.type]?.name || 'UNKNOWN';
        acc[typeName] = (acc[typeName] || 0) + 1;
        return acc;
    }, {});

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* System Overview */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <HardDrive size={20} />
                        File System Overview
                    </h3>
                    <div style={{ display: 'flex', gap: '0.5rem' }}>
                        <button
                            className={`btn btn-sm ${viewMode === 'blocks' ? 'btn-primary' : 'btn-secondary'}`}
                            onClick={() => setViewMode('blocks')}
                        >
                            Block View
                        </button>
                        <button
                            className={`btn btn-sm ${viewMode === 'hierarchy' ? 'btn-primary' : 'btn-secondary'}`}
                            onClick={() => setViewMode('hierarchy')}
                        >
                            Hierarchy
                        </button>
                    </div>
                </div>
                <div className="card-body">
                    <div className="grid grid-3" style={{ gap: '1.5rem', marginBottom: '2rem' }}>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-lg)',
                            border: '2px solid var(--accent-primary)'
                        }}>
                            <Database size={32} style={{ color: 'var(--accent-primary)', marginBottom: '0.5rem' }} />
                            <div style={{ fontSize: '0.875rem', color: 'var(--text-secondary)', marginBottom: '0.25rem' }}>
                                Total Capacity
                            </div>
                            <div style={{ fontSize: '1.75rem', fontWeight: '700', color: 'var(--text-primary)' }}>
                                {(totalSpace / (1024 * 1024)).toFixed(2)} MB
                            </div>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-lg)',
                            border: '2px solid var(--accent-success)'
                        }}>
                            <Zap size={32} style={{ color: 'var(--accent-success)', marginBottom: '0.5rem' }} />
                            <div style={{ fontSize: '0.875rem', color: 'var(--text-secondary)', marginBottom: '0.25rem' }}>
                                Used Space
                            </div>
                            <div style={{ fontSize: '1.75rem', fontWeight: '700', color: 'var(--text-primary)' }}>
                                {(usedSpace / (1024 * 1024)).toFixed(2)} MB
                            </div>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-lg)',
                            border: '2px solid var(--accent-warning)'
                        }}>
                            <TrendingUp size={32} style={{ color: 'var(--accent-warning)', marginBottom: '0.5rem' }} />
                            <div style={{ fontSize: '0.875rem', color: 'var(--text-secondary)', marginBottom: '0.25rem' }}>
                                Free Space
                            </div>
                            <div style={{ fontSize: '1.75rem', fontWeight: '700', color: 'var(--text-primary)' }}>
                                {(freeSpace / (1024 * 1024)).toFixed(2)} MB
                            </div>
                        </div>
                    </div>

                    {/* Space Utilization Bar */}
                    <div style={{ marginBottom: '2rem' }}>
                        <div style={{
                            display: 'flex',
                            justifyContent: 'space-between',
                            marginBottom: '0.5rem',
                            fontSize: '0.875rem',
                            color: 'var(--text-secondary)'
                        }}>
                            <span>Space Utilization</span>
                            <span>{((usedSpace / totalSpace) * 100).toFixed(2)}%</span>
                        </div>
                        <div style={{
                            height: '24px',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-full)',
                            overflow: 'hidden',
                            position: 'relative'
                        }}>
                            <div style={{
                                height: '100%',
                                width: `${(usedSpace / totalSpace) * 100}%`,
                                background: 'var(--gradient-primary)',
                                transition: 'width 0.5s ease',
                                position: 'relative',
                                overflow: 'hidden'
                            }}>
                                <div style={{
                                    position: 'absolute',
                                    top: 0,
                                    left: 0,
                                    width: '100%',
                                    height: '100%',
                                    background: 'linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.3), transparent)',
                                    animation: 'shimmer 2s infinite'
                                }}></div>
                            </div>
                        </div>
                    </div>

                    {/* Block Type Distribution */}
                    <div>
                        <h4 style={{
                            fontSize: '1rem',
                            marginBottom: '1rem',
                            color: 'var(--text-primary)'
                        }}>
                            Block Type Distribution
                        </h4>
                        <div className="grid grid-4" style={{ gap: '1rem' }}>
                            {Object.entries(blockTypeStats).map(([type, count]) => {
                                const typeInfo = Object.values(blockTypes).find(t => t.name === type) || blockTypes[0];
                                return (
                                    <div
                                        key={type}
                                        style={{
                                            padding: '1rem',
                                            background: typeInfo.bg,
                                            borderRadius: 'var(--radius-md)',
                                            border: `1px solid ${typeInfo.color}`,
                                            textAlign: 'center'
                                        }}
                                    >
                                        <div style={{
                                            fontSize: '1.5rem',
                                            fontWeight: '700',
                                            color: typeInfo.color,
                                            marginBottom: '0.25rem'
                                        }}>
                                            {count}
                                        </div>
                                        <div style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                            {type}
                                        </div>
                                    </div>
                                );
                            })}
                        </div>
                    </div>
                </div>
            </div>

            {/* Visual Block Map */}
            {viewMode === 'blocks' && (
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            <Layers size={20} />
                            Block Allocation Map
                        </h3>
                    </div>
                    <div className="card-body">
                        <div style={{
                            display: 'grid',
                            gridTemplateColumns: 'repeat(auto-fill, minmax(20px, 1fr))',
                            gap: '2px',
                            padding: '1rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            maxHeight: '400px',
                            overflowY: 'auto'
                        }}>
                            {blocks.slice(0, 1000).map((block, index) => {
                                const typeInfo = blockTypes[block.type] || blockTypes[0];
                                return (
                                    <div
                                        key={index}
                                        className="tooltip"
                                        style={{
                                            width: '20px',
                                            height: '20px',
                                            background: typeInfo.color,
                                            borderRadius: '2px',
                                            opacity: block.type === 0 ? 0.3 : 1,
                                            transition: 'all 0.2s',
                                            cursor: 'pointer',
                                            position: 'relative'
                                        }}
                                        onMouseEnter={(e) => {
                                            e.currentTarget.style.transform = 'scale(1.5)';
                                            e.currentTarget.style.zIndex = '10';
                                        }}
                                        onMouseLeave={(e) => {
                                            e.currentTarget.style.transform = 'scale(1)';
                                            e.currentTarget.style.zIndex = '1';
                                        }}
                                    >
                                        {block.refCount > 1 && (
                                            <div style={{
                                                position: 'absolute',
                                                top: '-4px',
                                                right: '-4px',
                                                width: '10px',
                                                height: '10px',
                                                background: 'var(--accent-danger)',
                                                borderRadius: '50%',
                                                fontSize: '6px',
                                                display: 'flex',
                                                alignItems: 'center',
                                                justifyContent: 'center',
                                                color: 'white',
                                                fontWeight: '700'
                                            }}>
                                                {block.refCount}
                                            </div>
                                        )}
                                        <span className="tooltip-text">
                                            Block {block.id} | {typeInfo.name} | Refs: {block.refCount}
                                        </span>
                                    </div>
                                );
                            })}
                        </div>
                    </div>
                </div>
            )}

            {/* File System Hierarchy */}
            {viewMode === 'hierarchy' && (
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            <Layers size={20} />
                            File System Hierarchy
                        </h3>
                    </div>
                    <div className="card-body">
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            fontFamily: 'var(--font-mono)',
                            fontSize: '0.875rem'
                        }}>
                            <div style={{ marginBottom: '1rem', color: 'var(--accent-primary)' }}>
                                📁 / (root)
                            </div>
                            {files.map((file, index) => (
                                <div
                                    key={index}
                                    style={{
                                        marginLeft: '2rem',
                                        marginBottom: '0.5rem',
                                        color: 'var(--text-secondary)',
                                        display: 'flex',
                                        alignItems: 'center',
                                        gap: '0.5rem'
                                    }}
                                >
                                    <span style={{ color: 'var(--accent-success)' }}>📄</span>
                                    <span>{file.name}</span>
                                    <span style={{ color: 'var(--text-tertiary)', fontSize: '0.75rem' }}>
                                        ({file.size} bytes, {file.blocks} blocks)
                                    </span>
                                </div>
                            ))}
                            {snapshots.length > 0 && (
                                <>
                                    <div style={{
                                        marginTop: '1rem',
                                        marginBottom: '0.5rem',
                                        color: 'var(--accent-warning)'
                                    }}>
                                        📸 .snapshots/
                                    </div>
                                    {snapshots.slice(0, 5).map((snapshot, index) => (
                                        <div
                                            key={index}
                                            style={{
                                                marginLeft: '2rem',
                                                marginBottom: '0.5rem',
                                                color: 'var(--text-secondary)',
                                                fontSize: '0.75rem'
                                            }}
                                        >
                                            ├─ {snapshot.name} ({snapshot.inodeCount} files)
                                        </div>
                                    ))}
                                </>
                            )}
                        </div>
                    </div>
                </div>
            )}

            {/* Quick Stats */}
            <div className="grid grid-4" style={{ gap: '1rem' }}>
                <div className="stat-card">
                    <Layers size={24} style={{ color: 'var(--accent-primary)', marginBottom: '0.5rem' }} />
                    <div className="stat-label">CoW Blocks</div>
                    <div className="stat-value">
                        {blocks.filter(b => b.isCow).length}
                    </div>
                </div>
                <div className="stat-card">
                    <Copy size={24} style={{ color: 'var(--accent-success)', marginBottom: '0.5rem' }} />
                    <div className="stat-label">Deduplicated</div>
                    <div className="stat-value">
                        {blocks.filter(b => b.isDeduplicated).length}
                    </div>
                </div>
                <div className="stat-card">
                    <Database size={24} style={{ color: 'var(--accent-warning)', marginBottom: '0.5rem' }} />
                    <div className="stat-label">Shared Blocks</div>
                    <div className="stat-value">
                        {blocks.filter(b => b.refCount > 1).length}
                    </div>
                </div>
                <div className="stat-card">
                    <Zap size={24} style={{ color: 'var(--accent-info)', marginBottom: '0.5rem' }} />
                    <div className="stat-label">Efficiency</div>
                    <div className="stat-value">
                        {((1 - (usedSpace / totalSpace)) * 100).toFixed(0)}%
                    </div>
                </div>
            </div>
        </div>
    );
};

export default FileSystemVisualizer;
