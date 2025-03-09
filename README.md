# Report Management System

A daemon-based solution for managing department reports in a manufacturing company. This system automates the collection, transfer, and backup of XML reports while providing transparency and accountability.

## Features

- **Automated File Transfer**: Collects reports from department managers and moves them to a dashboard directory
- **Nightly Backups**: Creates timestamped backups of all dashboard reports
- **Change Tracking**: Logs all file changes with user, file, and timestamp information
- **Directory Security**: Locks directories during critical operations to prevent modifications
- **Missing Report Detection**: Identifies departments that haven't submitted reports
- **Manual Control**: Supports triggering operations via signals

## Requirements

- Linux operating system (Debian/Ubuntu recommended)
- GCC compiler and development tools
- Root access for installation

## Installation

### From Source

1. Clone the repository or extract the archive:
   ```bash
   git clone https://github.com/itsJino/report-management.git
   cd report_management_system
   ```

2. Compile the daemon:
   ```bash
   make
   ```

3. Create initialization script:
   ```bash
   make init-script
   ```

4. Install the daemon:
   ```bash
   sudo make install
   ```

5. Start the daemon:
   ```bash
   sudo /etc/init.d/report_daemon start
   ```

### Directory Structure

The system creates the following directory structure:

```
/var/report_system/
├── upload/           # Department managers upload reports here
├── dashboard/        # Reports are transferred here for processing
├── backup/           # Backup storage location
└── logs/             # System logs directory
```

## Usage

### Report File Format

Department reports should be XML files with the following naming convention:
```
report_DepartmentName_YYYY-MM-DD.xml
```

For example:
```
report_Warehouse_2025-03-08.xml
report_Manufacturing_2025-03-08.xml
report_Sales_2025-03-08.xml
report_Distribution_2025-03-08.xml
```

### Report Submission

1. Department managers should save their XML reports to the upload directory:
   ```
   /var/report_system/upload/
   ```

2. Reports must be submitted by 11:30 PM each day.

3. The system will automatically:
   - Transfer reports to the dashboard directory at 1:00 AM
   - Create a backup of all reports
   - Check for any missing department reports

### Log Files

The system maintains detailed logs in the following files:

- **Operations Log**: `/var/report_system/logs/operations.log`
- **Error Log**: `/var/report_system/logs/error.log`
- **Change Log**: `/var/report_system/logs/changes.log`

### Manual Control

You can manually control the daemon with these commands:

- **Check Status**: `sudo /etc/init.d/report_daemon status`
- **Start Daemon**: `sudo /etc/init.d/report_daemon start`
- **Stop Daemon**: `sudo /etc/init.d/report_daemon stop`
- **Restart Daemon**: `sudo /etc/init.d/report_daemon restart`

For advanced control, you can use signals:

- Force immediate backup: `sudo kill -USR1 $(cat /var/run/report_daemon.pid)`
- Force immediate transfer: `sudo kill -USR2 $(cat /var/run/report_daemon.pid)`

## Troubleshooting

### Common Issues

1. **Permission Denied Errors**:
   - Ensure the daemon is running as root
   - Check directory permissions (upload: 777, dashboard: 755)

2. **Missing Reports**:
   - Check error logs for details about missing department reports
   - Verify that report files follow the correct naming convention

3. **Daemon Not Starting**:
   - Check for existing PID file: `/var/run/report_daemon.pid`
   - Verify installation paths and permissions

### Log Analysis

To check for errors:
```bash
grep ERROR /var/report_system/logs/error.log
```

To check for missing reports:
```bash
grep "Missing report" /var/report_system/logs/error.log
```

To see file changes by a specific user:
```bash
grep "User: username" /var/report_system/logs/changes.log
```

## Uninstallation

To remove the system:
```bash
sudo make uninstall
```

This will:
- Stop the daemon if running
- Remove the init script
- Remove the daemon executable

Note: This does not remove the data directories or logs.

## License

This software is provided under the MIT License. See LICENSE file for details.

## Contact

For support or questions, please contact your system administrator.