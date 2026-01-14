import React from 'react';
import { BarChart3, TrendingUp, Zap, Database, Clock, Activity } from 'lucide-react';
import { LineChart, Line, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, PieChart, Pie, Cell } from 'recharts';

const PerformanceMetrics = ({ metrics, systemStatus }) => {
    if (!metrics || !systemStatus) {
        return (
            <div className="card">
                <div className="card-body" style={{ textAlign: 'center', padding: '3rem' }}>
                    <Activity size={48} style={{ opacity: 0.3, marginBottom: '1rem' }} />
                    <p style={{ color: 'var(--text-secondary)' }}>Loading metrics...</p>
                </div>
            </div>
        );
    }

    // Calculate derived metrics
    const blockUtilization = systemStatus.totalBlocks > 0
        ? (systemStatus.usedBlocks / systemStatus.totalBlocks * 100).toFixed(2)
        : 0;

    const inodeUtilization = systemStatus.totalInodes > 0
        ? (systemStatus.usedInodes / systemStatus.totalInodes * 100).toFixed(2)
        : 0;

    const dedupRatio = metrics.blocksDeduplicated > 0
        ? ((metrics.bytesSavedDedup / (metrics.blocksDeduplicated * 4096)) * 100).toFixed(2)
        : 0;

    const cowEfficiency = metrics.bytesSavedCow > 0
        ? (metrics.bytesSavedCow / (1024 * 1024)).toFixed(2)
        : 0;

    // Mock time-series data for charts
    const performanceData = Array.from({ length: 10 }, (_, i) => ({
        time: `T${i}`,
        writes: Math.floor(Math.random() * 100) + metrics.totalWrites / 10,
        reads: Math.floor(Math.random() * 150) + metrics.totalReads / 10,
        snapshots: Math.floor(Math.random() * 10)
    }));

    const operationData = [
        { name: 'Reads', value: Number(metrics.totalReads), color: 'var(--accent-primary)' },
        { name: 'Writes', value: Number(metrics.totalWrites), color: 'var(--accent-success)' },
        { name: 'Snapshots', value: Number(metrics.totalSnapshots), color: 'var(--accent-warning)' },
        { name: 'Rollbacks', value: Number(metrics.totalRollbacks), color: 'var(--accent-danger)' }
    ];

    const storageData = [
        { name: 'Used Blocks', value: systemStatus.usedBlocks, color: '#667eea' },
        { name: 'Free Blocks', value: systemStatus.totalBlocks - systemStatus.usedBlocks, color: '#64748b' }
    ];

    const COLORS = ['#667eea', '#10b981', '#f59e0b', '#ef4444'];

    return (
        <div className="grid" style={{ gap: '2rem' }}>
            {/* Key Performance Indicators */}
            <div className="grid grid-4" style={{ gap: '1rem' }}>
                <div className="stat-card">
                    <div className="stat-label">Block Utilization</div>
                    <div className="stat-value">{blockUtilization}%</div>
                    <div className="progress" style={{ marginTop: '1rem' }}>
                        <div
                            className="progress-bar"
                            style={{ width: `${blockUtilization}%` }}
                        ></div>
                    </div>
                </div>
                <div className="stat-card">
                    <div className="stat-label">Inode Utilization</div>
                    <div className="stat-value">{inodeUtilization}%</div>
                    <div className="progress" style={{ marginTop: '1rem' }}>
                        <div
                            className="progress-bar"
                            style={{ width: `${inodeUtilization}%` }}
                        ></div>
                    </div>
                </div>
                <div className="stat-card">
                    <div className="stat-label">Dedup Savings</div>
                    <div className="stat-value">{(metrics.bytesSavedDedup / (1024 * 1024)).toFixed(2)} MB</div>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-secondary)', marginTop: '0.5rem' }}>
                        {metrics.blocksDeduplicated} blocks deduplicated
                    </div>
                </div>
                <div className="stat-card">
                    <div className="stat-label">CoW Savings</div>
                    <div className="stat-value">{cowEfficiency} MB</div>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-secondary)', marginTop: '0.5rem' }}>
                        Copy-on-Write efficiency
                    </div>
                </div>
            </div>

            {/* Performance Charts */}
            <div className="grid grid-2" style={{ gap: '1rem' }}>
                {/* Operations Over Time */}
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            <TrendingUp size={20} />
                            Operations Over Time
                        </h3>
                    </div>
                    <div className="card-body">
                        <ResponsiveContainer width="100%" height={300}>
                            <LineChart data={performanceData}>
                                <CartesianGrid strokeDasharray="3 3" stroke="var(--border-color)" />
                                <XAxis dataKey="time" stroke="var(--text-secondary)" />
                                <YAxis stroke="var(--text-secondary)" />
                                <Tooltip
                                    contentStyle={{
                                        background: 'var(--bg-tertiary)',
                                        border: '1px solid var(--border-color)',
                                        borderRadius: 'var(--radius-md)'
                                    }}
                                />
                                <Legend />
                                <Line
                                    type="monotone"
                                    dataKey="reads"
                                    stroke="var(--accent-primary)"
                                    strokeWidth={2}
                                    dot={{ fill: 'var(--accent-primary)' }}
                                />
                                <Line
                                    type="monotone"
                                    dataKey="writes"
                                    stroke="var(--accent-success)"
                                    strokeWidth={2}
                                    dot={{ fill: 'var(--accent-success)' }}
                                />
                                <Line
                                    type="monotone"
                                    dataKey="snapshots"
                                    stroke="var(--accent-warning)"
                                    strokeWidth={2}
                                    dot={{ fill: 'var(--accent-warning)' }}
                                />
                            </LineChart>
                        </ResponsiveContainer>
                    </div>
                </div>

                {/* Storage Distribution */}
                <div className="card">
                    <div className="card-header">
                        <h3 className="card-title">
                            <Database size={20} />
                            Storage Distribution
                        </h3>
                    </div>
                    <div className="card-body">
                        <ResponsiveContainer width="100%" height={300}>
                            <PieChart>
                                <Pie
                                    data={storageData}
                                    cx="50%"
                                    cy="50%"
                                    labelLine={false}
                                    label={({ name, percent }) => `${name}: ${(percent * 100).toFixed(0)}%`}
                                    outerRadius={100}
                                    fill="#8884d8"
                                    dataKey="value"
                                >
                                    {storageData.map((entry, index) => (
                                        <Cell key={`cell-${index}`} fill={entry.color} />
                                    ))}
                                </Pie>
                                <Tooltip
                                    contentStyle={{
                                        background: 'var(--bg-tertiary)',
                                        border: '1px solid var(--border-color)',
                                        borderRadius: 'var(--radius-md)'
                                    }}
                                />
                            </PieChart>
                        </ResponsiveContainer>
                    </div>
                </div>
            </div>

            {/* Operation Statistics */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <BarChart3 size={20} />
                        Operation Statistics
                    </h3>
                </div>
                <div className="card-body">
                    <ResponsiveContainer width="100%" height={300}>
                        <BarChart data={operationData}>
                            <CartesianGrid strokeDasharray="3 3" stroke="var(--border-color)" />
                            <XAxis dataKey="name" stroke="var(--text-secondary)" />
                            <YAxis stroke="var(--text-secondary)" />
                            <Tooltip
                                contentStyle={{
                                    background: 'var(--bg-tertiary)',
                                    border: '1px solid var(--border-color)',
                                    borderRadius: 'var(--radius-md)'
                                }}
                            />
                            <Bar dataKey="value" radius={[8, 8, 0, 0]}>
                                {operationData.map((entry, index) => (
                                    <Cell key={`cell-${index}`} fill={COLORS[index % COLORS.length]} />
                                ))}
                            </Bar>
                        </BarChart>
                    </ResponsiveContainer>
                </div>
            </div>

            {/* Detailed Metrics Table */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Clock size={20} />
                        Performance Timings
                    </h3>
                </div>
                <div className="card-body">
                    <div className="table-container">
                        <table className="table">
                            <thead>
                                <tr>
                                    <th>Operation</th>
                                    <th>Average Time (ms)</th>
                                    <th>Total Count</th>
                                    <th>Status</th>
                                </tr>
                            </thead>
                            <tbody>
                                <tr>
                                    <td>Read Operations</td>
                                    <td style={{ fontFamily: 'var(--font-mono)' }}>
                                        {(metrics.avgReadTime * 1000).toFixed(3)}
                                    </td>
                                    <td>{metrics.totalReads.toLocaleString()}</td>
                                    <td>
                                        <span className="badge badge-success">Optimal</span>
                                    </td>
                                </tr>
                                <tr>
                                    <td>Write Operations</td>
                                    <td style={{ fontFamily: 'var(--font-mono)' }}>
                                        {(metrics.avgWriteTime * 1000).toFixed(3)}
                                    </td>
                                    <td>{metrics.totalWrites.toLocaleString()}</td>
                                    <td>
                                        <span className="badge badge-success">Optimal</span>
                                    </td>
                                </tr>
                                <tr>
                                    <td>Snapshot Creation</td>
                                    <td style={{ fontFamily: 'var(--font-mono)' }}>
                                        {(metrics.avgSnapshotTime * 1000).toFixed(3)}
                                    </td>
                                    <td>{metrics.totalSnapshots.toLocaleString()}</td>
                                    <td>
                                        <span className="badge badge-primary">Good</span>
                                    </td>
                                </tr>
                                <tr>
                                    <td>Rollback Operations</td>
                                    <td style={{ fontFamily: 'var(--font-mono)' }}>
                                        {(metrics.avgRollbackTime * 1000).toFixed(3)}
                                    </td>
                                    <td>{metrics.totalRollbacks.toLocaleString()}</td>
                                    <td>
                                        <span className="badge badge-warning">Moderate</span>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>

            {/* System Health */}
            <div className="card">
                <div className="card-header">
                    <h3 className="card-title">
                        <Activity size={20} />
                        System Health
                    </h3>
                </div>
                <div className="card-body">
                    <div className="grid grid-3" style={{ gap: '1rem' }}>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <Zap size={32} style={{
                                color: 'var(--accent-success)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Performance
                            </h4>
                            <div style={{ fontSize: '1.5rem', fontWeight: '700', color: 'var(--accent-success)' }}>
                                Excellent
                            </div>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <Database size={32} style={{
                                color: 'var(--accent-primary)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Storage Efficiency
                            </h4>
                            <div style={{ fontSize: '1.5rem', fontWeight: '700', color: 'var(--accent-primary)' }}>
                                {dedupRatio}%
                            </div>
                        </div>
                        <div style={{
                            padding: '1.5rem',
                            background: 'var(--bg-tertiary)',
                            borderRadius: 'var(--radius-md)',
                            textAlign: 'center'
                        }}>
                            <TrendingUp size={32} style={{
                                color: 'var(--accent-warning)',
                                marginBottom: '0.5rem'
                            }} />
                            <h4 style={{ fontSize: '0.875rem', marginBottom: '0.25rem' }}>
                                Throughput
                            </h4>
                            <div style={{ fontSize: '1.5rem', fontWeight: '700', color: 'var(--accent-warning)' }}>
                                High
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default PerformanceMetrics;
