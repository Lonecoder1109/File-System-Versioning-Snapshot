import React, { useState, useEffect } from 'react';
import { Database, Layers, Copy, Zap } from 'lucide-react';

const BlockManager = ({ onRefresh }) => {
    const [blocks, setBlocks] = useState([]);
    const [filter, setFilter] = useState('all');
    const [selectedBlocks, setSelectedBlocks] = useState([]);
    const [loading, setLoading] = useState(true);

    const blockTypes = {
        0: { name: 'FREE', color: 'var(--text-tertiary)', bg: 'rgba(100, 116, 139, 0.1)' },
        1: { name: 'DATA', color: 'var(--accent-primary)', bg: 'rgba(102, 126, 234, 0.2)' },
        2: { name: 'INODE', color: 'var(--accent-success)', bg: 'rgba(16, 185, 129, 0.2)' },
        3: { name: 'METADATA', color: 'var(--accent-warning)', bg: 'rgba(245, 158, 11, 0.2)' },
        4: { name: 'SNAPSHOT', color: 'var(--accent-info)', bg: 'rgba(59, 130, 246, 0.2)' },
        5: { name: 'JOURNAL', color: '#f59e0b', bg: 'rgba(245, 158, 11, 0.2)' },
        6: { name: 'BTREE', color: '#8b5cf6', bg: 'rgba(139, 92, 246, 0.2)' },
        7: { name: 'DEDUP', color: '#ec4899', bg: 'rgba(236, 72, 153, 0.2)' }
    };

    // Fetch blocks from backend
    const fetchBlocks = async () => {
        try {
            setLoading(true);
            const res = await fetch('http://localhost:8080/api/blocks');
            const data = await res.json();
            if (data.success && Array.isArray(data.blocks)) {
                const mapped = data.blocks.map(b => ({
                    id: b.id,
                    type: b.type,                    
                    isCow: b.isCow === true || b.isCow === "true",         // CoW status
                    isDeduplicated: b.isDeduplicated === true || b.isDeduplicated === "true", // Dedup status
                    refCount: b.refCount || 1
                }));
                setBlocks(mapped);
            } else {
                setBlocks([]);
            }
        } catch (err) {
            console.error('Failed to fetch blocks:', err);
            setBlocks([]);
        } finally {
            setLoading(false);
        }
    };

    // Fetch on mount and whenever onRefresh changes
    useEffect(() => {
        fetchBlocks();
    }, [onRefresh]);

    const filteredBlocks = blocks.filter(block => {
        if (filter === 'all') return true;
        if (filter === 'used') return block.type !== 0;
        if (filter === 'free') return block.type === 0;
        if (filter === 'cow') return block.isCow;
        if (filter === 'dedup') return block.isDeduplicated;
        return true;
    });

    const stats = {
        total: blocks.length,
        used: blocks.filter(b => b.type !== 0).length,
        free: blocks.filter(b => b.type === 0).length,
        cow: blocks.filter(b => b.isCow).length,
        dedup: blocks.filter(b => b.isDeduplicated).length
    };

    const toggleBlockSelection = (blockId) => {
        setSelectedBlocks(prev =>
            prev.includes(blockId)
                ? prev.filter(id => id !== blockId)
                : [...prev, blockId]
        );
    };

    if (loading) return <div>Loading blocks...</div>;

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* Block Statistics */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Database size={20} />
                        Block Statistics
                    </h3>
                </div>
                <div className="card-body">
                    <div className="grid grid-4" style={{ gap: '1rem' }}>
                        {[
                            { label: 'Total Blocks', value: stats.total, color: 'var(--text-primary)' },
                            { label: 'Used', value: stats.used, color: 'var(--accent-primary)' },
                            { label: 'CoW Blocks', value: stats.cow, color: 'var(--accent-success)' },
                            { label: 'Deduplicated', value: stats.dedup, color: 'var(--accent-warning)' }
                        ].map((s, i) => (
                            <div key={i} style={{ textAlign: 'center', padding: '1rem' }}>
                                <div style={{ fontSize: '2rem', fontWeight: '700', color: s.color }}>
                                    {s.value}
                                </div>
                                <div style={{ fontSize: '0.875rem', color: 'var(--text-secondary)' }}>
                                    {s.label}
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            </div>

            {/* Block Filters */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Layers size={20} />
                        Block Visualization
                    </h3>
                    <div style={{ display: 'flex', gap: '0.5rem' }}>
                        {['all','used','free','cow','dedup'].map(f => (
                            <button
                                key={f}
                                className={`btn btn-sm ${filter === f ? 'btn-primary' : 'btn-secondary'}`}
                                onClick={() => setFilter(f)}
                            >
                                {f.charAt(0).toUpperCase() + f.slice(1)}
                            </button>
                        ))}
                    </div>
                </div>
                <div className="card-body">
                    {/* Block Grid Visualization */}
                    <div style={{
                        display: 'grid',
                        gridTemplateColumns: 'repeat(auto-fill, minmax(40px, 1fr))',
                        gap: '4px',
                        maxHeight: '400px',
                        overflowY: 'auto',
                        padding: '1rem',
                        background: 'var(--bg-tertiary)',
                        borderRadius: 'var(--radius-md)'
                    }}>
                        {filteredBlocks.slice(0, 500).map(block => {
                            const typeInfo = blockTypes[block.type] || blockTypes[0];
                            const isSelected = selectedBlocks.includes(block.id);

                            return (
                                <div
                                    key={block.id}
                                    onClick={() => toggleBlockSelection(block.id)}
                                    className="tooltip"
                                    style={{
                                        width: '40px',
                                        height: '40px',
                                        background: typeInfo.bg,
                                        border: `2px solid ${isSelected ? 'var(--accent-primary)' : 'transparent'}`,
                                        borderRadius: 'var(--radius-sm)',
                                        cursor: 'pointer',
                                        transition: 'all 0.2s',
                                        position: 'relative',
                                        display: 'flex',
                                        alignItems: 'center',
                                        justifyContent: 'center',
                                        fontSize: '0.625rem',
                                        fontWeight: '600',
                                        color: typeInfo.color
                                    }}
                                >
                                    {block.refCount > 1 && (
                                        <div style={{
                                            position: 'absolute',
                                            top: '-4px',
                                            right: '-4px',
                                            width: '16px',
                                            height: '16px',
                                            background: 'var(--accent-danger)',
                                            borderRadius: '50%',
                                            fontSize: '0.625rem',
                                            display: 'flex',
                                            alignItems: 'center',
                                            justifyContent: 'center',
                                            color: 'white'
                                        }}>
                                            {block.refCount}
                                        </div>
                                    )}
                                    {block.isCow && <Copy size={12} />}
                                    {block.isDeduplicated && <Zap size={12} />}
                                    <span className="tooltip-text">
                                        Block {block.id} | {typeInfo.name} | Refs: {block.refCount}
                                    </span>
                                </div>
                            );
                        })}
                    </div>

                    {/* Legend */}
                    <div style={{
                        marginTop: '1rem',
                        display: 'flex',
                        flexWrap: 'wrap',
                        gap: '1rem',
                        padding: '1rem',
                        background: 'var(--bg-tertiary)',
                        borderRadius: 'var(--radius-md)'
                    }}>
                        {Object.entries(blockTypes).map(([type, info]) => (
                            <div key={type} style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                                <div style={{
                                    width: '20px',
                                    height: '20px',
                                    background: info.bg,
                                    borderRadius: 'var(--radius-sm)'
                                }}></div>
                                <span style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                                    {info.name}
                                </span>
                            </div>
                        ))}
                    </div>
                </div>
            </div>

            {/* Selected Blocks Info */}
            {selectedBlocks.length > 0 && (
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            Selected Blocks ({selectedBlocks.length})
                        </h3>
                        <button
                            className="btn btn-sm btn-secondary"
                            onClick={() => setSelectedBlocks([])}
                        >
                            Clear Selection
                        </button>
                    </div>
                    <div className="card-body">
                        <div className="table-container">
                            <table className="table">
                                <thead>
                                    <tr>
                                        <th>Block ID</th>
                                        <th>Type</th>
                                        <th>Ref Count</th>
                                        <th>CoW</th>
                                        <th>Deduplicated</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    {blocks.filter(b => selectedBlocks.includes(b.id)).map(block => {
                                        const typeInfo = blockTypes[block.type] || blockTypes[0];
                                        return (
                                            <tr key={block.id}>
                                                <td style={{ fontFamily: 'var(--font-mono)' }}>{block.id}</td>
                                                <td>
                                                    <span className="badge badge-primary">{typeInfo.name}</span>
                                                </td>
                                                <td>{block.refCount}</td>
                                                <td>{block.isCow ? '✓' : '—'}</td>
                                                <td>{block.isDeduplicated ? '✓' : '—'}</td>
                                            </tr>
                                        );
                                    })}
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default BlockManager;