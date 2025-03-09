/**
 * @file file_operations.c
 * @brief Implementation of file transfer and monitoring functions
 */

 #include "report_system.h"

 /* Static variables for tracking directory state */
 time_t last_scan_time = 0;
 ReportFile* previous_files = NULL;
 int previous_file_count = 0;
 
 /**
  * Transfer reports from upload directory to dashboard directory
  * @return SUCCESS on success, FAILURE on error
  */
 int transfer_reports(void) {
     DIR *dir;
     struct dirent *entry;
     char src_path[MAX_PATH_LENGTH];
     char dest_path[MAX_PATH_LENGTH];
     int result = SUCCESS;
     
     log_operation("Starting report transfer from upload to dashboard");
     
     /* Open the upload directory */
     dir = opendir(UPLOAD_DIR);
     if (dir == NULL) {
         log_error("Failed to open upload directory: %s", strerror(errno));
         return FAILURE;
     }
     
     /* Process each file in the directory */
     while ((entry = readdir(dir)) != NULL) {
         /* Skip directory entries and non-XML files */
         if (entry->d_type == DT_DIR || 
             strstr(entry->d_name, REPORT_EXTENSION) == NULL) {
             continue;
         }
         
         /* Construct source and destination paths */
         snprintf(src_path, MAX_PATH_LENGTH, "%s/%s", UPLOAD_DIR, entry->d_name);
         snprintf(dest_path, MAX_PATH_LENGTH, "%s/%s", DASHBOARD_DIR, entry->d_name);
         
         /* Move the file */
         log_operation("Moving file: %s to %s", entry->d_name, DASHBOARD_DIR);
         if (move_file(src_path, dest_path) != SUCCESS) {
             log_error("Failed to move file %s to dashboard", entry->d_name);
             result = FAILURE;
             continue;
         }
         
         /* Log the transfer operation */
         char owner[MAX_USER_LENGTH];
         if (get_file_owner(dest_path, owner, MAX_USER_LENGTH) == SUCCESS) {
             log_file_change(owner, entry->d_name, "transfer");
         }
     }
     
     closedir(dir);
     return result;
 }
 
 /**
  * Check for missing department reports
  * @return Number of missing reports
  */
 int check_missing_reports(void) {
     char department_reports[4][MAX_PATH_LENGTH] = {
         DEPT_WAREHOUSE,
         DEPT_MANUFACTURING,
         DEPT_SALES,
         DEPT_DISTRIBUTION
     };
     int missing_count = 0;
     int found[4] = {0, 0, 0, 0};
     DIR *dir;
     struct dirent *entry;
     char department[MAX_USER_LENGTH];
     
     log_operation("Checking for missing department reports");
     
     /* Open the dashboard directory */
     dir = opendir(DASHBOARD_DIR);
     if (dir == NULL) {
         log_error("Failed to open dashboard directory: %s", strerror(errno));
         return 4; /* Assume all reports are missing */
     }
     
     /* Scan for report files */
     while ((entry = readdir(dir)) != NULL) {
         /* Skip directory entries and non-XML files */
         if (entry->d_type == DT_DIR || 
             strstr(entry->d_name, REPORT_EXTENSION) == NULL) {
             continue;
         }
         
         /* Extract department from filename */
         if (extract_department_from_filename(entry->d_name, department, MAX_USER_LENGTH) != NULL) {
             /* Mark the department as found */
             for (int i = 0; i < 4; i++) {
                 if (strcasecmp(department, department_reports[i]) == 0) {
                     found[i] = 1;
                     break;
                 }
             }
         }
     }
     
     closedir(dir);
     
     /* Log missing reports */
     for (int i = 0; i < 4; i++) {
         if (!found[i]) {
             log_error("Missing report from department: %s", department_reports[i]);
             missing_count++;
         }
     }
     
     log_operation("Missing report check completed, %d missing", missing_count);
     return missing_count;
 }
 
 /**
  * Extract department name from filename
  * Expected format: report_Department_YYYY-MM-DD.xml
  * 
  * @param filename The filename to parse
  * @param department Buffer to store extracted department
  * @param dept_size Size of the department buffer
  * @return Pointer to department buffer or NULL on error
  */
 char* extract_department_from_filename(const char* filename, char* department, size_t dept_size) {
     const char *prefix = REPORT_PREFIX;
     const char *start, *end;
     size_t length;
     
     /* Verify filename starts with the expected prefix */
     if (strncmp(filename, prefix, strlen(prefix)) != 0) {
         return NULL;
     }
     
     /* Find the start of department name (after prefix) */
     start = filename + strlen(prefix);
     
     /* Find the end of department name (before date) */
     end = strchr(start, '_');
     if (end == NULL) {
         /* Try with the extension if there's no date part */
         end = strstr(start, REPORT_EXTENSION);
         if (end == NULL) {
             return NULL;
         }
     }
     
     /* Calculate length and copy department name */
     length = end - start;
     if (length >= dept_size) {
         length = dept_size - 1;
     }
     
     strncpy(department, start, length);
     department[length] = '\0';
     
     return department;
 }
 
 /**
  * Monitor directory for changes
  * @return SUCCESS on success, FAILURE on error
  */
 int monitor_directory_changes(void) {
     ReportFile *current_files = NULL;
     int current_file_count = 0;
     int i, j;
     int found;
     
     /* Scan the upload directory */
     if (scan_directory(UPLOAD_DIR, &current_files, &current_file_count) != SUCCESS) {
         return FAILURE;
     }
     
     /* If this is the first scan, just save the results */
     if (previous_files == NULL) {
         previous_files = current_files;
         previous_file_count = current_file_count;
         last_scan_time = time(NULL);
         return SUCCESS;
     }
     
     /* Look for new or modified files */
     for (i = 0; i < current_file_count; i++) {
         found = 0;
         
         for (j = 0; j < previous_file_count; j++) {
             if (strcmp(current_files[i].filename, previous_files[j].filename) == 0) {
                 found = 1;
                 
                 /* Check if file was modified */
                 if (current_files[i].timestamp > previous_files[j].timestamp) {
                     /* Log file modification */
                     log_file_change(current_files[i].owner, 
                                    current_files[i].filename, 
                                    "modify");
                 }
                 
                 break;
             }
         }
         
         if (!found) {
             /* Log new file */
             log_file_change(current_files[i].owner, 
                            current_files[i].filename, 
                            "create");
         }
     }
     
     /* Look for deleted files */
     for (j = 0; j < previous_file_count; j++) {
         found = 0;
         
         for (i = 0; i < current_file_count; i++) {
             if (strcmp(previous_files[j].filename, current_files[i].filename) == 0) {
                 found = 1;
                 break;
             }
         }
         
         if (!found) {
             /* Log file deletion */
             log_file_change(previous_files[j].owner, 
                            previous_files[j].filename, 
                            "delete");
         }
     }
     
     /* Free previous file list and update with current scan */
     free_report_files(previous_files, previous_file_count);
     previous_files = current_files;
     previous_file_count = current_file_count;
     last_scan_time = time(NULL);
     
     return SUCCESS;
 }
 
 /**
  * Scan a directory and return information about all files
  * 
  * @param dir_path Path to the directory to scan
  * @param files Pointer to an array of ReportFile structures to populate
  * @param count Pointer to store the number of files found
  * @return SUCCESS on success, FAILURE on error
  */
 int scan_directory(const char* dir_path, ReportFile** files, int* count) {
     DIR *dir;
     struct dirent *entry;
     struct stat file_stat;
     int file_count = 0;
     int array_size = 10; /* Initial size, will grow as needed */
     
     /* Open the directory */
     dir = opendir(dir_path);
     if (dir == NULL) {
         log_error("Failed to open directory %s: %s", dir_path, strerror(errno));
         return FAILURE;
     }
     
     /* Allocate initial memory for files array */
     *files = (ReportFile*)malloc(array_size * sizeof(ReportFile));
     if (*files == NULL) {
         log_error("Memory allocation failed for file list");
         closedir(dir);
         return FAILURE;
     }
     
     /* Process each file in the directory */
     while ((entry = readdir(dir)) != NULL) {
         char full_path[MAX_PATH_LENGTH];
         
         /* Skip directory entries and hidden files */
         if (entry->d_type == DT_DIR || entry->d_name[0] == '.') {
             continue;
         }
         
         /* Construct the full path */
         snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", dir_path, entry->d_name);
         
         /* Get file information */
         if (stat(full_path, &file_stat) != 0) {
             log_error("Failed to get file stats for %s: %s", 
                       entry->d_name, strerror(errno));
             continue;
         }
         
         /* Resize the array if needed */
         if (file_count >= array_size) {
             array_size *= 2;
             ReportFile *new_files = (ReportFile*)realloc(*files, 
                                     array_size * sizeof(ReportFile));
             if (new_files == NULL) {
                 log_error("Memory reallocation failed for file list");
                 free(*files);
                 *files = NULL;
                 *count = 0;
                 closedir(dir);
                 return FAILURE;
             }
             *files = new_files;
         }
         
         /* Fill in file information */
         memcpy((*files)[file_count].path, full_path, MAX_PATH_LENGTH - 1);
         (*files)[file_count].path[MAX_PATH_LENGTH - 1] = '\0';

         memcpy((*files)[file_count].filename, entry->d_name, MAX_PATH_LENGTH - 1);
         (*files)[file_count].filename[MAX_PATH_LENGTH - 1] = '\0';
         
         (*files)[file_count].timestamp = file_stat.st_mtime;
         (*files)[file_count].size = file_stat.st_size;
         
         /* Get file owner */
         get_file_owner(full_path, (*files)[file_count].owner, MAX_USER_LENGTH);
         
         /* Extract department if it's a report file */
         if (strstr(entry->d_name, REPORT_EXTENSION) != NULL) {
             extract_department_from_filename(entry->d_name, 
                                            (*files)[file_count].department, 
                                            MAX_USER_LENGTH);
         } else {
             (*files)[file_count].department[0] = '\0';
         }
         
         file_count++;
     }
     
     closedir(dir);
     *count = file_count;
     
     return SUCCESS;
 }
 
 /**
  * Log file change to the change log
  * 
  * @param username Username who made the change
  * @param filename Filename that was changed
  * @param action Action performed (create, modify, delete)
  * @return SUCCESS on success, FAILURE on error
  */
 int log_file_change(const char* username, const char* filename, const char* action) {
     FILE *log_fp;
     time_t now;
     char time_str[MAX_TIME_LENGTH];
     
     /* Get current time */
     now = time(NULL);
     get_timestamp_string(now, time_str, MAX_TIME_LENGTH);
     
     /* Open the change log file for appending */
     log_fp = fopen(CHANGE_LOG, "a");
     if (log_fp == NULL) {
         log_error("Failed to open change log file: %s", strerror(errno));
         return FAILURE;
     }
     
     /* Write the log entry */
     fprintf(log_fp, "[%s] User: %s, File: %s, Action: %s\n", 
             time_str, username, filename, action);
     
     /* Close the log file */
     fclose(log_fp);
     
     return SUCCESS;
 }
 
 /**
  * Get the owner of a file
  * 
  * @param path Path to the file
  * @param owner Buffer to store the owner name
  * @param owner_size Size of the owner buffer
  * @return SUCCESS on success, FAILURE on error
  */
 int get_file_owner(const char* path, char* owner, size_t owner_size) {
     struct stat file_stat;
     struct passwd *pwd;
     
     /* Get file information */
     if (stat(path, &file_stat) != 0) {
         log_error("Failed to get file stats for %s: %s", path, strerror(errno));
         return FAILURE;
     }
     
     /* Get user information */
     pwd = getpwuid(file_stat.st_uid);
     if (pwd == NULL) {
         log_error("Failed to get owner for %s: %s", path, strerror(errno));
         snprintf(owner, owner_size, "%d", file_stat.st_uid);
         return FAILURE;
     }
     
     /* Copy owner name */
     strncpy(owner, pwd->pw_name, owner_size - 1);
     owner[owner_size - 1] = '\0';
     
     return SUCCESS;
 }
 
 /**
  * Free memory allocated for report files
  * 
  * @param files Pointer to the array of ReportFile structures
  * @param count Number of files in the array
  */
// Fix for the unused parameter warning
 void free_report_files(ReportFile* files, int count) {
     /* Suppress unused parameter warning */
     (void)count;
    
     if (files != NULL) {
         free(files);
     }
 } 
 /**
  * Move a file from source to destination
  * 
  * @param source Source file path
  * @param destination Destination file path
  * @return SUCCESS on success, FAILURE on error
  */
 int move_file(const char* source, const char* destination) {
     /* First try to rename the file (works if on same filesystem) */
     if (rename(source, destination) == 0) {
         return SUCCESS;
     }
     
     /* If rename fails, try copy and delete */
     if (copy_file(source, destination) == SUCCESS) {
         /* Delete the source file */
         if (unlink(source) != 0) {
             log_error("Failed to delete source file after copy: %s", strerror(errno));
             return FAILURE;
         }
         return SUCCESS;
     }
     
     return FAILURE;
 }
 
 /**
  * Copy a file from source to destination
  * 
  * @param source Source file path
  * @param destination Destination file path
  * @return SUCCESS on success, FAILURE on error
  */
 int copy_file(const char* source, const char* destination) {
     int src_fd, dest_fd;
     char buffer[4096];
     ssize_t bytes_read, bytes_written;
     int result = SUCCESS;
     
     /* Open source file for reading */
     src_fd = open(source, O_RDONLY);
     if (src_fd == -1) {
         log_error("Failed to open source file %s: %s", source, strerror(errno));
         return FAILURE;
     }
     
     /* Open destination file for writing, create if it doesn't exist */
     dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if (dest_fd == -1) {
         log_error("Failed to open destination file %s: %s", 
                   destination, strerror(errno));
         close(src_fd);
         return FAILURE;
     }
     
     /* Copy data */
     while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
         bytes_written = write(dest_fd, buffer, bytes_read);
         if (bytes_written != bytes_read) {
             log_error("Failed to write to destination file: %s", strerror(errno));
             result = FAILURE;
             break;
         }
     }
     
     /* Check for read error */
     if (bytes_read == -1) {
         log_error("Failed to read from source file: %s", strerror(errno));
         result = FAILURE;
     }
     
     /* Close files */
     close(src_fd);
     close(dest_fd);
     
     return result;
 }
 
 /**
  * Check if a file is a valid XML report
  * This is a basic check - in a real system you might want more validation
  * 
  * @param filepath Path to the file to check
  * @return TRUE if valid, FALSE if not
  */
 int is_valid_xml_report(const char* filepath) {
     FILE *fp;
     char buffer[1024];
     int is_valid = FALSE;
     
     /* Check file extension */
     if (strstr(filepath, REPORT_EXTENSION) == NULL) {
         return FALSE;
     }
     
     /* Open the file */
     fp = fopen(filepath, "r");
     if (fp == NULL) {
         return FALSE;
     }
     
     /* Read the first few bytes to check for XML header */
     if (fgets(buffer, sizeof(buffer), fp) != NULL) {
         /* Basic check for XML format - should start with <?xml */
         if (strstr(buffer, "<?xml") != NULL) {
             is_valid = TRUE;
         }
     }
     
     fclose(fp);
     return is_valid;
 }