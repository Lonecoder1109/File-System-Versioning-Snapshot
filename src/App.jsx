import React, { useState, useEffect } from 'react';
import {
    HardDrive, Database, FileText, Camera, Clock, Layers,
    GitBranch, Tag, Settings, BarChart3, Plus, Trash2,
    RotateCcw, Save, FolderOpen, Edit, Copy, Share2,
    Zap, Shield, TrendingUp, Activity
} from 'lucide-react';
import './index.css';
import FileSystemVisualizer from './components/FileSystemVisualizer';
import BlockManager from './components/BlockManager';
import SnapshotManager from './components/SnapshotManager';
import VersionManager from './components/VersionManager';
import PerformanceMetrics from './components/PerformanceMetrics';
import FileOperations from './components/FileOperations';

const API_BASE = '/api';

function App() {
    const [activeTab, setActiveTab] = useState('overview');
    const [systemStatus, setSystemStatus] = useState(null);
    const [files, setFiles] = useState([]);
    const [blocks, setBlocks] = useState([]);
    const [snapshots, setSnapshots] = useState([]);
    const [loading, setLoading] = useState(true);
    const [refreshInterval, setRefreshInterval] = useState(2000);

    // Fetch system status
    const fetchStatus = async () => {
        try {
            const response = await fetch(`${API_BASE}/status`);
            const data = await response.json();
            if (data.success) {
                setSystemStatus(data);
            }
        } catch (error) {
            console.error('Failed to fetch status:', error);
        }
    };

    // Fetch files
    const fetchFiles = async () => {
        try {
            const response = await fetch(`${API_BASE}/files`);
            const data = await response.json();
            if (data.success) {
                setFiles(data.files);
            }
        } catch (error) {
            console.error('Failed to fetch files:', error);
        }
    };

    // Fetch blocks
    const fetchBlocks = async () => {
        try {
            const response = await fetch(`${API_BASE}/blocks`);
            const data = await response.json();
            if (data.success) {
                setBlocks(data.blocks);
            }
        } catch (error) {
            console.error('Failed to fetch blocks:', error);
        }
    };

    // Fetch snapshots
    const fetchSnapshots = async () => {
        try {
            const response = await fetch(`${API_BASE}/snapshots`);
            const data = await response.json();
            if (data.success) {
                setSnapshots(data.snapshots);
            }
        } catch (error) {
            console.error('Failed to fetch snapshots:', error);
        }
    };

    // Refresh all data
    const refreshData = async () => {
        setLoading(true);
        await Promise.all([
            fetchStatus(),
            fetchFiles(),
            fetchBlocks(),
            fetchSnapshots()
        ]);
        setLoading(false);
    };

    // Initial load only (no auto-refresh)
    useEffect(() => {
        refreshData();
    }, []);

    const stats = systemStatus ? [
        {
            label: 'Total Blocks',
            value: systemStatus.totalBlocks,
            icon: <Database size={24} />,
            color: 'var(--gradient-primary)'
        },
        {
            label: 'Used Blocks',
            value: systemStatus.usedBlocks,
            icon: <HardDrive size={24} />,
            color: 'var(--gradient-success)'
        },
        {
            label: 'Files',
            value: systemStatus.usedInodes,
            icon: <FileText size={24} />,
            color: 'var(--gradient-warning)'
        },
        {
            label: 'Snapshots',
            value: systemStatus.snapshotCount,
            icon: <Camera size={24} />,
            color: 'var(--gradient-info)'
        },
        {
            label: 'Total Writes',
            value: systemStatus.metrics?.totalWrites || 0,
            icon: <Edit size={24} />,
            color: 'var(--gradient-secondary)'
        },
        {
            label: 'Deduplicated',
            value: systemStatus.metrics?.blocksDeduplicated || 0,
            icon: <Copy size={24} />,
            color: 'var(--gradient-success)'
        }
    ] : [];

    return (
        <div className="app">
            {/* Header */}
            <header className="header">
                <div className="container">
                    <div className="header-content">
                        <div className="logo">
                            <div className="logo-icon">
                                <HardDrive size={24} color="white" />
                            </div>
                            <span>Advanced File System Simulator</span>
                        </div>
                        <div style={{ display: 'flex', gap: '1rem', alignItems: 'center' }}>
                            <button className="btn btn-primary btn-sm" onClick={refreshData}>
                                <RotateCcw size={16} />
                                Refresh Data
                            </button>
                            <div style={{ color: 'var(--text-secondary)', fontSize: '0.875rem' }}>
                                <Activity size={16} style={{ display: 'inline', marginRight: '0.5rem' }} />
                                Manual Refresh Mode
                            </div>
                        </div>
                    </div>
                </div>
            </header>

            {/* Main Content */}
            <main className="container">
                {/* Stats Grid */}
                <div className="stats-grid">
                    {stats.map((stat, index) => (
                        <div key={index} className="stat-card">
                            <div className="stat-label">{stat.label}</div>
                            <div className="stat-value">{stat.value.toLocaleString()}</div>
                            <div className="stat-icon" style={{ background: stat.color }}>
                                {stat.icon}
                            </div>
                        </div>
                    ))}
                </div>

                {/* Tabs */}
                <div className="tabs">
                    <button
                        className={`tab ${activeTab === 'overview' ? 'active' : ''}`}
                        onClick={() => setActiveTab('overview')}
                    >
                        <TrendingUp size={16} />
                        Overview
                    </button>
                    <button
                        className={`tab ${activeTab === 'blocks' ? 'active' : ''}`}
                        onClick={() => setActiveTab('blocks')}
                    >
                        <Database size={16} />
                        Block Manager
                    </button>
                    <button
                        className={`tab ${activeTab === 'files' ? 'active' : ''}`}
                        onClick={() => setActiveTab('files')}
                    >
                        <FileText size={16} />
                        File Operations
                    </button>
                    <button
                        className={`tab ${activeTab === 'snapshots' ? 'active' : ''}`}
                        onClick={() => setActiveTab('snapshots')}
                    >
                        <Camera size={16} />
                        Snapshots
                    </button>
                    <button
                        className={`tab ${activeTab === 'versions' ? 'active' : ''}`}
                        onClick={() => setActiveTab('versions')}
                    >
                        <GitBranch size={16} />
                        Versions
                    </button>
                    <button
                        className={`tab ${activeTab === 'metrics' ? 'active' : ''}`}
                        onClick={() => setActiveTab('metrics')}
                    >
                        <BarChart3 size={16} />
                        Performance
                    </button>
                </div>

                {/* Tab Content */}
                {loading && activeTab === 'overview' ? (
                    <div style={{ display: 'flex', justifyContent: 'center', padding: '4rem' }}>
                        <div className="spinner"></div>
                    </div>
                ) : (
                    <>
                        {activeTab === 'overview' && (
                            <FileSystemVisualizer
                                blocks={blocks}
                                files={files}
                                snapshots={snapshots}
                                systemStatus={systemStatus}
                            />
                        )}

                        {activeTab === 'blocks' && (
                            <BlockManager
                                blocks={blocks}
                                onRefresh={refreshData}
                            />
                        )}

                        {activeTab === 'files' && (
                            <FileOperations
                                files={files}
                                onRefresh={refreshData}
                            />
                        )}

                        {activeTab === 'snapshots' && (
                            <SnapshotManager
                                snapshots={snapshots}
                                onRefresh={refreshData}
                            />
                        )}

                        {activeTab === 'versions' && (
                            <VersionManager
                                files={files}
                                onRefresh={refreshData}
                            />
                        )}

                        {activeTab === 'metrics' && (
                            <PerformanceMetrics
                                metrics={systemStatus?.metrics}
                                systemStatus={systemStatus}
                            />
                        )}
                    </>
                )}
            </main>
        </div>
    );
}

export default App;
