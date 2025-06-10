// emmc.h
#pragma once
#include <stdbool.h> 

extern int NumSystemRecords;
extern int DBErrorCounter;

void emmc_init(void);
void creat_table(void);
void read_csv_row( const char *FILE_PATH, int target_row);
bool GetNumberOfSystemRecords(const char *filepath);
bool Write_default_System_json(const char *filepath);
void Crea_csv_table(void);