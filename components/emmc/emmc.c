#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "driver/sdmmc_default_configs.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "emmc.h" // Include your own header
#include "configuration.h"
#include "cJSON.h"

#define MOUNT_POINT "/emmc"

static const char *TAG = "EMMC";

#define SDIO_CLK GPIO_NUM_7
#define SDIO_CMD GPIO_NUM_6
#define SDIO_D0  GPIO_NUM_5
#define SDIO_D1  GPIO_NUM_17
#define SDIO_D2  GPIO_NUM_18
#define SDIO_D3  GPIO_NUM_4
#define SDIO_CD  GPIO_NUM_NC
#define SDIO_WP  GPIO_NUM_NC

int row_no = 0;
#define MAX_LINE_LENGTH 512
// Global variables to mimic the C# class members
int NumSystemRecords = 0;
int DBErrorCounter = 0;


void emmc_init(void)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    host.slot = SDMMC_HOST_SLOT_1;
    host.flags = SDMMC_HOST_FLAG_DDR;

    sdmmc_slot_config_t slot_config = {
        .clk = SDIO_CLK,
        .cmd = SDIO_CMD,
        .d0 = SDIO_D0,
        .d1 = SDIO_D1,
        .d2 = SDIO_D2,
        .d3 = SDIO_D3,
        .cd = SDIO_CD,
        .wp = SDIO_WP,
        .width = 4,
        .flags = 0,
    };

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    ESP_LOGI(TAG, "Mounting eMMC...");

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount eMMC: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "eMMC mounted successfully.");
    sdmmc_card_print_info(stdout, card);     
}


void append_systeam_table(const char *filepath, const char*buffer )
{
    FILE *f = fopen(filepath, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    int a = row_no;
    if (row_no > 0 && a==row_no)
    {
		fprintf(f, "%s", buffer);
		fclose(f); 
		row_no++;
        ESP_LOGI( TAG,"File written: %s", filepath);
	}
}

void read_csv_row( const char *FILE_PATH, int target_row) {

    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        printf("Error opening file: %s\n", FILE_PATH);
        return;
    }

    char line[MAX_LINE_LENGTH];
    int current_row = 0;

    while (fgets(line, sizeof(line), file)) {
        current_row++;

        if (current_row == target_row) {
            // Print the row
            printf("Row %d: %s", target_row, line);
            break;
        }
    }

    if (current_row < target_row) {
        printf("Row %d not found in file.\n", target_row);
    }

    fclose(file);
}

void creat_systeam_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "serialNumber,deviceId,idsystem,GatewayAppRev,ActiveDevices,SerialNumber1,SerialNumber2,SerialNumber3,SerialNumber4,SerialNumber5,SerialNumber6,SerialNumber7,SerialNumber8,SerialNumber9,SerialNumber10,SerialNumber11,SerialNumber12,SerialNumber13,SerialNumber14,SoftwareId1,SoftwareId2,SoftwareId3,SoftwareId4,SoftwareId5,SoftwareId6,SoftwareId7,SoftwareId8,SoftwareId9,SoftwareId10,SoftwareId11,SoftwareId12,SoftwareId13,SoftwareId14,TEAltBuild1,TEAltBuild2,TEAltBuild3,TEAltBuild4,TEAltBuild5,TEAltBuild7,TEAltBuild8,TEAltBuild9,TEAltBuild10,TEAltBuild11,TEAltBuild11,TEAltBuild12,TEAltBuild13,TEAltBuild14,date_last_change,sent_to_cloud,sent_timeStamp,");
    fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "System table header written: %s", filepath);
}

void creat_setup_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idSetup,SerisalNumber,TimeStamp,Setup,sent_to_cloud,sent_timeStamp");
    fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "Setup table header written: %s", filepath);
}
void creat_realtime_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idrealtime,SerisalNumber,MachineName,EventType,EventDate,TimeStamp,FormulaNumber,FormulaName,weight,Alarm,ProgrammedPumps,PumpInterfaceStatus,PumRunStatus,CycleCounter,sent_to_cloud,sent_timeStamp");
    fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "realtime table header written: %s", filepath);
}
void creat_record_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idRecord,SerisalNumber,StartTimeStamp,EndTimeStamp,ForNum,Status,weight,StoppedIncompleteWeight,AmountPumped,Alarm,CycleCounter,sent_to_cloud,sent_timeStamp");
    fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "record table header written: %s", filepath);
}

void creat_firmware_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idFirmware,Status1,Status2,Status3,Status4,Status5,Status6,Status7,Status8,Status9,Status10,Status11,Status12,Status13,Status14,PercentComplete1,PercentComplete2,PercentComplete3,PercentComplete4,PercentComplete5,PercentComplete6,PercentComplete7,PercentComplete8,PercentComplete9,PercentComplete10,PercentComplete11,PercentComplete12,PercentComplete13,PercentComplete14,date_last_change,sent_Timestamp,currentVersion1,currentVersion2,currentVersion3,currentVersion4,currentVersion5,currentVersion6,currentVersion7,currentVersion8,currentVersion9,currentVersion10,currentVersion11,currentVersion12,currentVersion13,currentVersion14,NewVersion1,NewVersion2,NewVersion3,NewVersion4,NewVersion5,NewVersion6,NewVersion7,NewVerions8,NewVerions9,NewVersion10,NewVersion11,NewVersion12,NewVersion13,NewVersion14");
    fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "firmware table header written: %s", filepath);
}

void creat_Activitylog_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idActivitylog,SerisalNumber,TimeStamp,ForNum,Triggers,Pump,sent_to_cloud,send_timestamp");
     fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "activity table header written: %s", filepath);
}

void creat_Timezone_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idTimeZone,TimeZoneName,TimeStamp");
     fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "TimeZone table header written: %s", filepath);
}
void creat_XDD_HYC_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idSetup,SerisalNumber,TimeStamp,setup,append_number,ack_status,date_last_change,sent_to_cloud,send_timestamp");
     fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "XDD_HYC table header written: %s", filepath);
}
void creat_XDD_TEC_table(const char *filepath)
{
	remove(filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "idSetup,SerisalNumber,TimeStamp,setup,append_number,ack_status,date_last_change");
     fclose(f); // Always close the file
    row_no = 1;
    ESP_LOGI(TAG, "XDD_TEC table header written: %s", filepath);
}

bool GetNumberOfSystemRecords(const char *filepath) {
    // Define the struct inside the function
    struct SystemData {
        int idsystem;
        char GatewayAppRev[16];
        int ActiveDevices;
        char SerialNumber[14][16];
        char SoftwareId[14][64];
        int TEAltBuild[14];
        char date_last_change[32];
        int sent_to_cloud;
        char sent_timeStamp[32];
    };

    struct SystemData sysData;

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        DBErrorCounter++;
        return false;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);

    // Allocate buffer and read file content
    char *buffer = malloc(len + 1);
    if (!buffer) {
        fclose(file);
        ESP_LOGE(TAG, "Memory allocation failed");
        DBErrorCounter++;
        return false;
    }

    fread(buffer, 1, len, file);
    buffer[len] = '\0';
    fclose(file);

    // Parse JSON
    cJSON *json = cJSON_Parse(buffer);
    free(buffer);

    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        DBErrorCounter++;
        return false;
    }

    cJSON *eData = cJSON_GetObjectItem(json, "eData");
    if (!eData || !cJSON_IsObject(eData)) {
        ESP_LOGW(TAG, "eData not found or is not an object");
        cJSON_Delete(json);
        DBErrorCounter++;
        return false;
    }

    // Extract fields
    cJSON *item = NULL;

    item = cJSON_GetObjectItem(eData, "idsystem");
    if (cJSON_IsNumber(item)) {
        sysData.idsystem = item->valueint;
    }

    item = cJSON_GetObjectItem(eData, "GatewayAppRev");
    if (cJSON_IsString(item)) {
        strncpy(sysData.GatewayAppRev, item->valuestring, sizeof(sysData.GatewayAppRev) - 1);
        sysData.GatewayAppRev[sizeof(sysData.GatewayAppRev) - 1] = '\0';
    }

    item = cJSON_GetObjectItem(eData, "ActiveDevices");
    if (cJSON_IsNumber(item)) {
        sysData.ActiveDevices = item->valueint;
    }

    for (int i = 0; i < 14; i++) {
        char key[32];
        snprintf(key, sizeof(key), "SerialNumber%d", i + 1);
        item = cJSON_GetObjectItem(eData, key);
        if (cJSON_IsString(item)) {
            strncpy(sysData.SerialNumber[i], item->valuestring, sizeof(sysData.SerialNumber[i]) - 1);
            sysData.SerialNumber[i][sizeof(sysData.SerialNumber[i]) - 1] = '\0';
        }

        snprintf(key, sizeof(key), "SoftwareId%d", i + 1);
        item = cJSON_GetObjectItem(eData, key);
        if (cJSON_IsString(item)) {
            strncpy(sysData.SoftwareId[i], item->valuestring, sizeof(sysData.SoftwareId[i]) - 1);
            sysData.SoftwareId[i][sizeof(sysData.SoftwareId[i]) - 1] = '\0';
        }

        snprintf(key, sizeof(key), "TEAltBuild%d", i + 1);
        item = cJSON_GetObjectItem(eData, key);
        if (cJSON_IsNumber(item)) {
            sysData.TEAltBuild[i] = item->valueint;
        }
    }

    item = cJSON_GetObjectItem(eData, "date_last_change");
    if (cJSON_IsString(item)) {
        strncpy(sysData.date_last_change, item->valuestring, sizeof(sysData.date_last_change) - 1);
        sysData.date_last_change[sizeof(sysData.date_last_change) - 1] = '\0';
    }

    item = cJSON_GetObjectItem(eData, "sent_to_cloud");
    if (cJSON_IsNumber(item)) {
        sysData.sent_to_cloud = item->valueint;
    }

    item = cJSON_GetObjectItem(eData, "sent_timeStamp");
    if (cJSON_IsString(item)) {
        strncpy(sysData.sent_timeStamp, item->valuestring, sizeof(sysData.sent_timeStamp) - 1);
        sysData.sent_timeStamp[sizeof(sysData.sent_timeStamp) - 1] = '\0';
    }

    // Process sysData as needed...

    cJSON_Delete(json);
    return true;
}

bool Write_default_System_json(const char *filepath) {
    struct {
        int idsystem;
        char GatewayAppRev[10];
        int ActiveDevices;
        char SerialNumber[14][10];
        char SoftwareId[14][51];
        int TEAltBuild[14];
        char date_last_change[20];
        int sent_to_cloud;
        char sent_timeStamp[20];
    } system_data;

    // Initialize the structure with default values
    system_data.idsystem = 1;
    snprintf(system_data.GatewayAppRev, sizeof(system_data.GatewayAppRev), "0");
    system_data.ActiveDevices = 0;
    for (int i = 0; i < 14; i++) {
        snprintf(system_data.SerialNumber[i], sizeof(system_data.SerialNumber[i]), "000000000");
        snprintf(system_data.SoftwareId[i], sizeof(system_data.SoftwareId[i]), "00000000000000000000000000000000000000000000000000");
        system_data.TEAltBuild[i] = 0;
    }
    snprintf(system_data.date_last_change, sizeof(system_data.date_last_change), "ABC111117");
    system_data.sent_to_cloud = 0;
    snprintf(system_data.sent_timeStamp, sizeof(system_data.sent_timeStamp), "ABC111117");

    // Create the JSON structure
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "dataType", "System");

    cJSON *mData = cJSON_CreateObject();
    cJSON_AddStringToObject(mData, "serialNumber", "000000000");
    cJSON_AddStringToObject(mData, "deviceId", "00000000000000000000000000000000");
    cJSON_AddItemToObject(root, "mData", mData);

    cJSON *eData = cJSON_CreateObject();
    cJSON_AddNumberToObject(eData, "idsystem", system_data.idsystem);
    cJSON_AddStringToObject(eData, "GatewayAppRev", system_data.GatewayAppRev);
    cJSON_AddNumberToObject(eData, "ActiveDevices", system_data.ActiveDevices);

    for (int i = 0; i < 14; i++) {
        char key[20];
        snprintf(key, sizeof(key), "SerialNumber%d", i + 1);
        cJSON_AddStringToObject(eData, key, system_data.SerialNumber[i]);
    }

    for (int i = 0; i < 14; i++) {
        char key[20];
        snprintf(key, sizeof(key), "SoftwareId%d", i + 1);
        cJSON_AddStringToObject(eData, key, system_data.SoftwareId[i]);
    }

    for (int i = 0; i < 14; i++) {
        char key[20];
        snprintf(key, sizeof(key), "TEAltBuild%d", i + 1);
        cJSON_AddNumberToObject(eData, key, system_data.TEAltBuild[i]);
    }

    cJSON_AddStringToObject(eData, "date_last_change", system_data.date_last_change);
    cJSON_AddNumberToObject(eData, "sent_to_cloud", system_data.sent_to_cloud);
    cJSON_AddStringToObject(eData, "sent_timeStamp", system_data.sent_timeStamp);

    cJSON_AddItemToObject(root, "eData", eData);

    // Convert JSON object to string
    char *json_string = cJSON_Print(root);

    if (!json_string) {
        cJSON_Delete(root);
        return false;  // failed to print JSON string
    }

    // Write JSON string to file
    FILE *file = fopen(filepath, "w");
    if (!file) {
        printf("Failed to open file for writing.\n");
        free(json_string);
        cJSON_Delete(root);
        return false;
    }

    if (fputs(json_string, file) == EOF) {
        printf("Failed to write JSON string to file.\n");
        fclose(file);
        free(json_string);
        cJSON_Delete(root);
        return false;
    }
    fclose(file);
    free(json_string);
    cJSON_Delete(root);
    return true;
}

void Crea_csv_table(void)
{
	creat_systeam_table(MOUNT_POINT"/SYSTEM_table.csv");
	creat_setup_table(MOUNT_POINT"/setup.csv");
	creat_realtime_table(MOUNT_POINT"/realtime.csv");
	creat_record_table(MOUNT_POINT"/record.csv");
	creat_firmware_table(MOUNT_POINT"/activitylog.csv");
	creat_Activitylog_table(MOUNT_POINT"/firmware.csv");
	creat_Timezone_table(MOUNT_POINT"/timezone.csv");
	creat_XDD_HYC_table(MOUNT_POINT"/xdd_hyc_setup.csv");
	creat_XDD_TEC_table(MOUNT_POINT"/xdd_tecc_setup.csv");	
}