#ifndef STACK_HIGH_WATERMARK_USAGE_HPP
#define STACK_HIGH_WATERMARK_USAGE_HPP

// singleton class that you can add freertos handles to, and on prompt it will print the stack high watermarks for all tasks added to it or the high watermark for a specific task

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

class StackHighWatermarkUsage {
public:
    /**
     * @brief Get the singleton instance of the class
     * @return Reference to the singleton instance
     */
    static StackHighWatermarkUsage& getInstance();

    /**
     * @brief Add a task to monitor
     * @param task The task handle to monitor
     * @return True if added successfully, false if the task is already being monitored or if the list is full
     */
    bool addTaskHandle(TaskHandle_t task);

    /**
     * @brief Remove a task from monitoring
     * @param task The task handle to remove
     * @return True if removed successfully, false if the task was not found in the list
     */
    bool removeTaskHandle(TaskHandle_t task);

    /**
     * @brief Print the stack high watermarks for all monitored tasks
     * @return True if the watermarks were printed successfully, false otherwise
     */
    void printAllHighWatermarks();

    /**
     * @brief Print the stack high watermark for a specific task
     * @param task The task handle for which to print the high watermark
     * @return True if the task was found and the high watermark was printed, false otherwise
     */
    bool printHighWatermarkForTask(TaskHandle_t task);

private:
    StackHighWatermarkUsage();

    int8_t findHandleIndex(TaskHandle_t taskHandle) const;

    // List of tasks to monitor
    TaskHandle_t tasks[4]; // maximum number of tasks to monitor
};



#endif