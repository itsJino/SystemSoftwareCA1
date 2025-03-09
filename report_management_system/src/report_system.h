/**
 * @file report_system.h
 * @brief Main header file for the manufacturing company report system
 * 
 * This header defines the core structures and functions for the report management
 * daemon that handles uploading, transfer, and backup of department reports.
 */

 #ifndef REPORT_SYSTEM_H
 #define REPORT_SYSTEM_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <signal.h>
 #include <syslog.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <sys/wait.h>
 #include <fcntl.h>
 #include <time.h>
 #include <dirent.h>
 #include <errno.h>
 #include <libgen.h>
 #include <pwd.h>
 #include <grp.h>
 #include <limits.h>
 
 /* Department definitions */
 #define DEPT_WAREHOUSE    "Warehouse"
 #define DEPT_MANUFACTURING "Manufacturing"
 #define DEPT_SALES        "Sales"
 #define DEPT_DISTRIBUTION "Distribution"
 
 /* File naming conventions */
 #define REPORT_EXTENSION  ".xml"
 #define REPORT_PREFIX     "report_"
 
 /* Path definitions - these would normally be in a config file */
 #define UPLOAD_DIR      "/var/report_system/upload"
 #define DASHBOARD_DIR   "/var/report_system/dashboard"
 #define BACKUP_DIR      "/var/report_system/backup"
 #define LOG_DIR         "/var/report_system/logs"
 #define PID_FILE        "/var/run/report_daemon.pid"
 #define LOCK_FILE       "/var/run/report_daemon.lock"
 #define CHANGE_LOG      "/var/report_system/logs/changes.log"
 #define ERROR_LOG       "/var/report_system/logs/error.log"
 #define OPERATION_LOG   "/var/report_system/logs/operations.log"
 #define FIFO_PATH       "/var/report_system/ipc_pipe"
 
 /* Time settings */
 #define TRANSFER_HOUR   1    /* 1:00 AM */
 #define TRANSFER_MINUTE 0
 #define UPLOAD_DEADLINE_HOUR 23   /* 11:30 PM */
 #define UPLOAD_DEADLINE_MINUTE 30
 
 /* Permission settings */
 #define UPLOAD_PERMISSIONS    0777
 #define DASHBOARD_PERMISSIONS 0755
 #define LOCKED_PERMISSIONS    0000
 
 /* Return codes */
 #define SUCCESS 0
 #define FAILURE -1
 
 /* Boolean definitions */
 #define TRUE  1
 #define FALSE 0
 
 /* Maximum buffer sizes */
 #define MAX_PATH_LENGTH 1024
 #define MAX_LINE_LENGTH 2048
 #define MAX_USER_LENGTH 256
 #define MAX_TIME_LENGTH 64
 
 /* IPC message types */
 #define MSG_BACKUP_START     1
 #define MSG_BACKUP_COMPLETE  2
 #define MSG_TRANSFER_START   3
 #define MSG_TRANSFER_COMPLETE 4
 #define MSG_ERROR            5
 
 /**
  * @struct ReportFile
  * @brief Structure to hold information about a report file
  */
 typedef struct {
     char path[MAX_PATH_LENGTH];       /* Full path to the file */
     char filename[MAX_PATH_LENGTH];   /* Just the filename */
     char department[MAX_USER_LENGTH]; /* Department name */
     time_t timestamp;                 /* Last modification time */
     char owner[MAX_USER_LENGTH];      /* Owner of the file */
     int size;                         /* File size in bytes */
 } ReportFile;
 
 /**
  * @struct ChangeRecord
  * @brief Structure to log changes to report files
  */
 typedef struct {
     char username[MAX_USER_LENGTH]; /* User who made the change */
     char filename[MAX_PATH_LENGTH]; /* File that was changed */
     char action[MAX_USER_LENGTH];   /* Action performed (create, modify, delete) */
     time_t timestamp;               /* When the change occurred */
 } ChangeRecord;
 
 /**
  * @struct IPCMessage
  * @brief Structure for inter-process communication
  */
 typedef struct {
     int type;                        /* Message type */
     pid_t sender_pid;                /* Sender process ID */
     int status;                      /* Status code */
     char message[MAX_LINE_LENGTH];   /* Additional message text */
 } IPCMessage;
 
 /* Daemon Initialization Functions */
 int daemon_init(void);
 int create_pid_file(void);
 int check_singleton(void);
 void daemon_cleanup(void);
 void signal_handler(int sig);
 void setup_signal_handlers(void);
 
 /* Core Operation Functions */
 int transfer_reports(void);
 int backup_dashboard(void);
 int lock_directories(void);
 int unlock_directories(void);
 int check_missing_reports(void);
 
 /* File Monitoring Functions */
 int monitor_directory_changes(void);
 int log_file_change(const char* username, const char* filename, const char* action);
 int get_file_owner(const char* path, char* owner, size_t owner_size);
 int scan_directory(const char* dir_path, ReportFile** files, int* count);
 
 /* Directory Management Functions */
 int create_directory_if_not_exists(const char* path);
 int set_directory_permissions(const char* path, mode_t mode);
 int is_directory_empty(const char* path);
 
 /* IPC Functions */
 int setup_ipc(void);
 int cleanup_ipc(void);
 int send_ipc_message(IPCMessage* msg);
 int receive_ipc_message(IPCMessage* msg);
 
 /* Logging Functions */
 void log_error(const char* format, ...);
 void log_operation(const char* format, ...);
 void log_change(ChangeRecord* record);
 
 /* Utility Functions */
 char* get_timestamp_string(time_t timestamp, char* buffer, size_t buffer_size);
 int is_valid_xml_report(const char* filepath);
 char* extract_department_from_filename(const char* filename, char* department, size_t dept_size);
 int copy_file(const char* source, const char* destination);
 int move_file(const char* source, const char* destination);
 void free_report_files(ReportFile* files, int count);
 
 /* Static variables for tracking directory state - these would typically be 
    defined in file_operations.c, but are declared here for reference */
 extern time_t last_scan_time;
 extern ReportFile* previous_files;
 extern int previous_file_count;
 
 #endif /* REPORT_SYSTEM_H */