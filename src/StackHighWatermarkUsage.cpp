#include "StackHighWatermarkUsage.hpp"
#include "serial.h"

StackHighWatermarkUsage& StackHighWatermarkUsage::getInstance() {
    static StackHighWatermarkUsage instance;
    return instance;
}

StackHighWatermarkUsage::StackHighWatermarkUsage() {
    for (uint8_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        tasks[i] = nullptr;
    }
}

int8_t StackHighWatermarkUsage::findHandleIndex(TaskHandle_t taskHandle) const {
    for (uint8_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        if (tasks[i] == taskHandle) {
            return i;
        }
    }
    return -1; // Not found
}

bool StackHighWatermarkUsage::addTaskHandle(TaskHandle_t task) {
    if (task == nullptr || task == NULL) return false;

    int8_t index = findHandleIndex(task);
    if (index != -1) { // task already exists in the list
        return false;
    }

    for (uint8_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        if (tasks[i] == nullptr || tasks[i] == NULL) { // find the first empty slot
            tasks[i] = task;
            return true; // successfully added
        }
    }

    // list is full
    return false;
}

bool StackHighWatermarkUsage::removeTaskHandle(TaskHandle_t task) {
    int8_t index = findHandleIndex(task);
    if (index != -1) {
        tasks[index] = nullptr; // remove the task
        return true; // successfully removed
    }
    return false; // task not found
}

void StackHighWatermarkUsage::printAllHighWatermarks() {
    for (uint8_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        if (tasks[i] != nullptr && tasks[i] != NULL) {
            configSTACK_DEPTH_TYPE freeWords = uxTaskGetStackHighWaterMark(tasks[i]);
            const char* taskName = pcTaskGetName(tasks[i]);
            serial_print_P(F("Task '")); 
            serial_print(taskName);
            serial_print_P(F("' has stack high watermark of "));
            serial_print_u16(freeWords * sizeof(StackType_t)); 
            serial_println_P(F(" bytes free."));
        }
    }
}

bool StackHighWatermarkUsage::printHighWatermarkForTask(TaskHandle_t task) {
    if (task == nullptr || task == NULL) return false;

    int8_t index = findHandleIndex(task);
    if (index != -1) {
        configSTACK_DEPTH_TYPE freeWords = uxTaskGetStackHighWaterMark(task);
        const char* taskName = pcTaskGetName(task);
        serial_print_P(F("Task '")); 
        serial_print(taskName);
        serial_print_P(F("' has stack high watermark of "));
        serial_print_u16(freeWords * sizeof(StackType_t)); 
        serial_println_P(F(" bytes."));
        return true;
    }

    // task not found in the list
    return false;
}