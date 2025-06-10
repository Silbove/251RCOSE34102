#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define INITIAL_READY_QUEUE_CAPACITY 10 // �ʱ� �غ� ť �뷮
#define INITIAL_WAITING_QUEUE_CAPACITY 5 // �ʱ� ��� ť �뷮 (�� �̻� ������ ����)
// #define MAX_IO_REQUESTS 3 // ���μ����� �ִ� I/O ��û �� (���ŵ�)
#define TIME_QUANTUM 3 // ���� �κ� �ð� �Ҵ緮

// ���μ��� ���� ����
typedef enum {
    NEW, // ���� ������
    READY, // ���� �غ� �Ϸ�
    RUNNING, // CPU���� ���� ��
    // WAITING, // I/O ��� �� (���ŵ�)
    TERMINATED // ���� �Ϸ�
} ProcessState;

// ���μ��� ����ü ����
typedef struct {
    int id; // ���μ��� ID
    int arrival_time; // ���� �ð�
    int cpu_burst_time; // �� CPU ����Ʈ �ð�
    int priority; // �켱����

    // I/O ���� �ʵ� ���ŵ�
    // int num_io_requests; 
    // int io_burst_time[MAX_IO_REQUESTS];
    // int io_request_time[MAX_IO_REQUESTS];

    int remaining_cpu_time; // ���� CPU ���� �ð�
    // int io_index; // ���� ó�� ���� I/O ��û �ε��� (���ŵ�)
    // int io_remaining_time; // ���� I/O �ð� (���ŵ�)
    int last_executed_time; // ���������� CPU�� �Ҵ���� �ð� (���� �ڵ忡���� ������ ����)
    int start_time; // ���μ����� ó�� CPU�� �Ҵ���� �ð�
    int completion_time; // �Ϸ� �ð�

    ProcessState state; // ���� ���μ��� ����
} Process;

// �ý��� ť ���� ����ü (���� schedule �Լ����� ���� ������ ����)
typedef struct {
    Process** ready_queue; // �غ� ť (���μ��� ������ �迭)
    int ready_queue_size; // �غ� ť ���� ũ��
    int ready_queue_capacity; // �غ� ť �뷮

    // Waiting Queue ���� �ʵ� ���ŵ�
    // Process** waiting_queue; 
    // int waiting_queue_size;
    // int waiting_queue_capacity;
} SystemConfig;

// ���� ���� (config() �Լ��� ���� ������ �ùķ��̼� �Ű�����)
int num_processes_global = 0; // ��ü ���μ��� ��
int max_arrival_time_global = 0; // �ִ� ���� �ð�
int max_cpu_burst_global = 0; // �ִ� CPU ����Ʈ �ð�
int max_priority_global = 0; // �ִ� �켱����
// int max_io_burst_global = 0; // �ִ� I/O ����Ʈ �ð� (���ŵ�)


// ���ο� ���μ��� ����
Process* create_process(int id, int max_arrival_time, int max_cpu_burst, int max_priority /*, int max_io_burst ���ŵ� */) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    if (!new_process) return NULL; // �޸� �Ҵ� ���� �� NULL ��ȯ

    new_process->id = id;
    new_process->arrival_time = rand() % (max_arrival_time + 1); // 0���� max_arrival_time���� ���� ����
    new_process->cpu_burst_time = (rand() % max_cpu_burst) + 1; // 1���� max_cpu_burst���� ���� ����
    new_process->priority = rand() % (max_priority + 1); // 0���� max_priority���� ���� ���� (���� ���ڰ� ���� �켱����)
    new_process->state = NEW; // �ʱ� ���´� NEW

    new_process->remaining_cpu_time = new_process->cpu_burst_time; // ���� CPU �ð� �ʱ�ȭ
    // I/O ���� �ʵ� �ʱ�ȭ ���ŵ�
    // new_process->io_index = 0; 
    // new_process->io_remaining_time = 0; 
    new_process->last_executed_time = -1; // ������ ���� �ð� �ʱ�ȭ
    new_process->start_time = -1; // ���� �ð� �ʱ�ȭ
    new_process->completion_time = -1; // �Ϸ� �ð� �ʱ�ȭ

    // I/O ��û ���� ���� ���ŵ�
    // new_process->num_io_requests = rand() % (MAX_IO_REQUESTS + 1); 
    // for (int i = 0; i < new_process->num_io_requests; i++) {
    //     new_process->io_burst_time[i] = (rand() % max_io_burst) + 1;
    //     int request_time;
    //     if (new_process->cpu_burst_time == 1) {
    //         request_time = 0;
    //     }
    //     else {
    //         do {
    //             request_time = rand() % new_process->cpu_burst_time;
    //         } while (request_time == 0); 
    //     }
    //     new_process->io_request_time[i] = request_time;
    // }

    return new_process;
}

// �ý��� ���� �Լ� (���� ���� �Ű������� �ڵ����� ����)
SystemConfig* config() {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    if (!config) return NULL; // �޸� �Ҵ� ���� �� NULL ��ȯ

    config->ready_queue_capacity = INITIAL_READY_QUEUE_CAPACITY; // �غ� ť �ʱ� �뷮 ����
    config->ready_queue_size = 0; // �غ� ť ũ�� �ʱ�ȭ
    config->ready_queue = (Process**)malloc(config->ready_queue_capacity * sizeof(Process*)); // �غ� ť �޸� �Ҵ�

    // Waiting Queue ���� �ʵ� �ʱ�ȭ ���ŵ�
    // config->waiting_queue_capacity = INITIAL_WAITING_QUEUE_CAPACITY;
    // config->waiting_queue_size = 0;
    // config->waiting_queue = (Process**)malloc(config->waiting_queue_capacity * sizeof(Process*));

    // ���� �ùķ��̼� �Ű������� �ڵ����� ����
    num_processes_global = 5;      // �⺻ ���μ��� ��
    max_arrival_time_global = 15;   // �⺻ �ִ� ���� �ð�
    max_cpu_burst_global = 25;      // �⺻ �ִ� CPU ����Ʈ �ð�
    max_priority_global = 5;        // �⺻ �ִ� �켱���� (0-5)
    // max_io_burst_global = 10;       // �⺻ �ִ� I/O ����Ʈ �ð� (���ŵ�)

    printf("\nSystem environment configuration automatically set:\n");
    printf("  Number of Processes: %d\n", num_processes_global);
    printf("  Max Arrival Time: %d\n", max_arrival_time_global);
    printf("  Max CPU Burst Time: %d\n", max_cpu_burst_global);
    printf("  Max Priority: %d\n", max_priority_global);
    // printf("  Max I/O Burst Time: %d\n", max_io_burst_global); // ���ŵ�
    return config;
}

void print_gantt_chart_formatted(int* gantt_array, int total_time_units) {
    printf("\n  Formatted Gantt Chart:\n");
    if (total_time_units == 0) {
        printf("    (No execution recorded)\n"); // ���� ��� ����
        return;
    }

    // "    CPU : "�� ���� (���� ����)
    const int LABEL_PREFIX_WIDTH = 8;

    // CPU ���� ���� (�ִ� ������ ���̷� �Ҵ�)
    // �� ��� "P#(duration)" �Ǵ� "Idle(duration)"�� �ִ� 20�� + ������ '-' 1��
    // �� �ð� ���� * (�ִ� ��� ���� + ������) + ������
    char* cpu_line_buffer = (char*)malloc(total_time_units * 21 + 100);
    if (!cpu_line_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for cpu_line_buffer.\n");
        return;
    }
    cpu_line_buffer[0] = '\0'; // ���� �ʱ�ȭ

    int current_cpu_char_pos = 0; // cpu_line_buffer �� ���� ���� ��ġ (���λ� ����)

    int last_pid = -2; // ���� ���μ��� ID (�ʱⰪ�� ��ȿ���� ���� PID)

    // CPU ���� ���۸� ä��� �� �ð� ������ ���� ��ġ ���
    for (int t = 0; t < total_time_units; t++) {
        // ���� �ð� ������ PID
        int current_pid_at_t = gantt_array[t];

        // ���ο� ����� ���۵Ǵ� ��� (���� PID�� �ٸ��ų� ù ��° �ð� ������ ���)
        if (current_pid_at_t != last_pid || t == 0) {
            if (t > 0) { // ù ����� �ƴϸ� ������ �߰�
                strcat(cpu_line_buffer, "-");
                current_cpu_char_pos++;
            }

            last_pid = current_pid_at_t;

            // ���� ����� ���� �ð� ���
            int block_duration = 0;
            for (int k = t; k < total_time_units && gantt_array[k] == last_pid; k++) {
                block_duration++;
            }

            char block_str[20]; // "P#(duration)" �Ǵ� "Idle(duration)" ���ڿ�
            int block_len;
            if (last_pid != -1) { // ���μ��� ���� ���
                block_len = snprintf(block_str, sizeof(block_str), "P%d(%d)", last_pid, block_duration);
            }
            else { // ���� ���
                block_len = snprintf(block_str, sizeof(block_str), "Idle(%d)", block_duration);
            }

            strcat(cpu_line_buffer, block_str);
            current_cpu_char_pos += block_len;

            // ���� t�� ����� ������ �̵��Ͽ� ���� �ݺ����� �� ����� �����ϵ��� ��
            // (�̹� ó���� �ð� ������ �ǳʶٱ� ����)
            t += block_duration - 1;
        }
    }
    cpu_line_buffer[current_cpu_char_pos] = '\0';

    // ���˵� ��Ʈ ��Ʈ ���
    // Time ���� ��� �ڵ带 �����߽��ϴ�.
    printf("    CPU : %s\n", cpu_line_buffer);

    // �Ҵ�� �޸� ����
    free(cpu_line_buffer);
}


// �����ٸ� �Լ�
void schedule(Process** processes_arr, int num_processes_arr, const char* algorithm, int preemptive, float* avg_tat, float* avg_wt) {
    int time = 0, completed = 0;
    long total_turnaround_time = 0, total_waiting_time = 0; // �����÷ο� ������ ���� long ���
    int gantt_capacity = 1000; // �ʱ� ��Ʈ ��Ʈ �뷮
    int gantt_size = 0; // ��ϵ� ���� �ð� ���� ��
    int* gantt_pid = malloc(gantt_capacity * sizeof(int)); // �� �ð� ������ ���� ���� ���μ��� PID�� ������ �迭

    if (!gantt_pid) {
        fprintf(stderr, "Error: Failed to allocate memory for gantt_pid.\n"); // ����: gantt_pid �޸� �Ҵ翡 �����߽��ϴ�.
        return;
    }

    // �� �ùķ��̼� ���� �� ���μ��� ���� �ʱ�ȭ
    for (int i = 0; i < num_processes_arr; i++) {
        processes_arr[i]->remaining_cpu_time = processes_arr[i]->cpu_burst_time;
        // I/O ���� �ʱ�ȭ ���ŵ�
        // processes_arr[i]->io_index = 0;
        // processes_arr[i]->io_remaining_time = 0;
        processes_arr[i]->last_executed_time = -1;
        processes_arr[i]->start_time = -1;
        processes_arr[i]->completion_time = -1;
        processes_arr[i]->state = NEW; // ��� ���μ����� NEW ���·� �ʱ�ȭ
    }

    // Round Robin�� ���� ���� ���� (�Լ� ȣ�� ���� ���� ������ ����)
    static int rr_index_static = 0; // �� ������ �Լ� ȣ�� ���� ���� �����ؾ� �մϴ�.
    if (strcmp(algorithm, "RR") == 0) {
        rr_index_static = 0; // RR �ùķ��̼� ���� �� �ε��� �ʱ�ȭ
    }


    while (completed < num_processes_arr) {
        // I/O ó�� �� WAITING -> READY ��ȯ ���� ���ŵ�
        /*
        for (int i = 0; i < num_processes_arr; i++) {
            Process* p = processes_arr[i];
            if (p->state == WAITING) {
                p->io_remaining_time--;
                if (p->io_remaining_time <= 0) {
                    p->state = READY;
                }
            }
        }
        */

        // NEW -> READY ��ȯ (���� �ð� ����)
        for (int i = 0; i < num_processes_arr; i++) {
            Process* p = processes_arr[i];
            if (p->state == NEW && p->arrival_time <= time) {
                p->state = READY;
            }
        }

        // ������ ���μ��� ����
        Process* selected = NULL;

        // Ready ������ ���μ����� �߿��� ���� (RR ����)
        if (strcmp(algorithm, "RR") != 0) {
            for (int i = 0; i < num_processes_arr; i++) {
                Process* p = processes_arr[i];
                if (p->state == READY) {
                    if (selected == NULL) {
                        selected = p;
                    }
                    else {
                        if (strcmp(algorithm, "FCFS") == 0) {
                            if (p->arrival_time < selected->arrival_time) {
                                selected = p;
                            }
                        }
                        else if (strcmp(algorithm, "SJF") == 0) {
                            if (p->remaining_cpu_time < selected->remaining_cpu_time) {
                                selected = p;
                            }
                        }
                        else if (strcmp(algorithm, "PRIORITY") == 0) {
                            // ���� �켱���� ���ڰ� ���� �켱������� ����
                            if (p->priority < selected->priority) {
                                selected = p;
                            }
                        }
                    }
                }
            }
        }
        else { // ���� �κ� �˰��� ó�� (���� ����)
            selected = NULL;
            int initial_rr_index = rr_index_static;
            int found_rr = 0;

            do {
                if (processes_arr[rr_index_static]->state == READY) {
                    selected = processes_arr[rr_index_static];
                    found_rr = 1;
                    break;
                }
                rr_index_static = (rr_index_static + 1) % num_processes_arr;
            } while (rr_index_static != initial_rr_index); // ��� ���μ����� �� ���� ���Ҵ��� Ȯ��

            if (!found_rr) { // Ready Queue�� ������ ���μ����� ������
                time++;
                // ��Ʈ ��Ʈ�� ���� �ð� ���
                if (gantt_size >= gantt_capacity) { // �ʿ�� ���Ҵ�
                    gantt_capacity *= 2;
                    gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                    if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // ����: ���Ҵ� ����.
                }
                gantt_pid[gantt_size++] = -1; // -1�� ���� ���¸� ��Ÿ��
                if (time > gantt_capacity * num_processes_arr * 2) { // ���� ��ġ
                    fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n"); // ���: �ùķ��̼� �ð��� ���� �ѵ��� �ʰ��߽��ϴ�. �ߴ��մϴ�.
                    break;
                }
                continue; // ���� �ð� ������ �̵�
            }
        }


        if (selected == NULL) { // ������ ���μ����� ������ (��� TERMINATED ����)
            time++;
            // ��Ʈ ��Ʈ�� ���� �ð� ���
            if (gantt_size >= gantt_capacity) { // �ʿ�� ���Ҵ�
                gantt_capacity *= 2;
                gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // ����: ���Ҵ� ����.
            }
            gantt_pid[gantt_size++] = -1; // -1�� ���� ���¸� ��Ÿ��
            if (time > gantt_capacity * num_processes_arr * 2) { // ���� ��ġ
                fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n"); // ���: �ùķ��̼� �ð��� ���� �ѵ��� �ʰ��߽��ϴ�. �ߴ��մϴ�.
                break;
            }
            continue; // ���� �ð� ������ �̵�
        }

        if (selected->start_time == -1) {
            selected->start_time = time;
        }
        selected->state = RUNNING;


        int execute_duration = 1; // �⺻ ���� �ð��� 1 �ð� ����
        if (strcmp(algorithm, "RR") == 0) {
            execute_duration = (selected->remaining_cpu_time < TIME_QUANTUM) ? selected->remaining_cpu_time : TIME_QUANTUM;
            // RR�� ���, ���� ���μ����� ���ʸ� �̸� ����
            rr_index_static = (rr_index_static + 1) % num_processes_arr;
        }
        else if (!preemptive) {
            // ������ �˰����� �� �� ���õǸ� �Ϸ�� ������ ���� (I/O �����Ƿ� ���� �ð� ��� ����)
            execute_duration = selected->remaining_cpu_time;
            // I/O ��û ���� ���� ���ŵ�
        }

        // ���μ��� ���� (execute_duration ��ŭ)
        for (int t_unit = 0; t_unit < execute_duration; t_unit++) {
            if (gantt_size >= gantt_capacity) { // �ʿ�� ���Ҵ�
                gantt_capacity *= 2;
                gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // ����: ���Ҵ� ����.
            }
            gantt_pid[gantt_size++] = selected->id; // ��Ʈ ��Ʈ�� ���� ���μ��� ���

            selected->remaining_cpu_time--;
            time++;

            // I/O ��û ó�� ���� ���ŵ�
            /*
            if (selected->io_index < selected->num_io_requests &&
                (selected->cpu_burst_time - selected->remaining_cpu_time) == selected->io_request_time[selected->io_index]) {
                selected->io_remaining_time = selected->io_burst_time[selected->io_index];
                selected->state = WAITING;
                selected->io_index++;
                break; // I/O ��û �߻� �� ���� ���� �ߴ�
            }
            */

            // ���μ��� ����
            if (selected->remaining_cpu_time == 0) {
                selected->completion_time = time;
                selected->state = TERMINATED;
                completed++;
                break; // ���μ��� �Ϸ� �� ���� ���� �ߴ�
            }

            // ������ �˰��򿡼� ���� Ȯ��
            if (preemptive && (strcmp(algorithm, "SJF") == 0 || strcmp(algorithm, "PRIORITY") == 0)) {
                Process* potential_preemptor = NULL;
                for (int j = 0; j < num_processes_arr; j++) {
                    Process* p_check = processes_arr[j];
                    if (p_check->state == NEW && p_check->arrival_time <= time) { // ���� ������ ���μ��� READY�� ��ȯ
                        p_check->state = READY;
                    }
                    if (p_check->state == READY) {
                        if (potential_preemptor == NULL) {
                            potential_preemptor = p_check;
                        }
                        else {
                            if (strcmp(algorithm, "SJF") == 0) {
                                if (p_check->remaining_cpu_time < selected->remaining_cpu_time) { // selected�� ��
                                    potential_preemptor = p_check;
                                }
                            }
                            else if (strcmp(algorithm, "PRIORITY") == 0) {
                                // ���� �켱���� ���ڰ� ���� �켱������� ����
                                if (p_check->priority < selected->priority) { // selected�� ��
                                    potential_preemptor = p_check;
                                }
                            }
                        }
                    }
                }

                if (potential_preemptor != NULL && potential_preemptor->id != selected->id) {
                    if ((strcmp(algorithm, "SJF") == 0 && potential_preemptor->remaining_cpu_time < selected->remaining_cpu_time) ||
                        (strcmp(algorithm, "PRIORITY") == 0 && potential_preemptor->priority < selected->priority)) {
                        selected->state = READY; // ���� ���μ����� READY�� �ǵ���
                        goto preempt_break; // ���� ������ �������� ���� �����ٸ� ����Ŭ�� �̵�
                    }
                }
            }
        }
    preempt_break:; // ���� �߻� �� ���� ����

        // ���� ���õ� ���μ����� �Ϸ���� �ʾҰ� I/O ��� ���µ� �ƴϸ� �ٽ� READY��
        // I/O ��� ���°� �����Ƿ� ������ ����ȭ��
        if (selected != NULL && selected->state == RUNNING) {
            selected->state = READY; // ���� ���� ���� ���μ����� Ready�� �ǵ���
        }

        if (time > gantt_capacity * num_processes_arr * 2) { // ���� ��ġ: �����ϰ� �� �ùķ��̼� ����
            fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n");
            break;
        }
    }

    // TAT, WT ���
    int actual_completed_count = 0;
    for (int i = 0; i < num_processes_arr; i++) {
        if (processes_arr[i]->state == TERMINATED) {
            int tat = processes_arr[i]->completion_time - processes_arr[i]->arrival_time;
            // WT = TAT - �� CPU ����Ʈ �ð� (I/O �ð��� ������� ����)
            int wt = tat - processes_arr[i]->cpu_burst_time;

            if (wt < 0) wt = 0; // ��� �ð��� ������ �� �� ����

            total_turnaround_time += tat;
            total_waiting_time += wt;
            actual_completed_count++;
        }
    }

    if (actual_completed_count > 0) {
        *avg_tat = (float)total_turnaround_time / actual_completed_count;
        *avg_wt = (float)total_waiting_time / actual_completed_count;
    }
    else {
        *avg_tat = 0.0f;
        *avg_wt = 0.0f;
    }


    // ���ο� ����ȭ�� �Լ��� ����Ͽ� ��Ʈ ��Ʈ ���
    print_gantt_chart_formatted(gantt_pid, gantt_size);

    free(gantt_pid);
}

// �� �Լ�
void evaluate(Process** original_processes, int num_processes) {
    const char* algorithms[] = { "FCFS", "SJF", "SJF", "PRIORITY", "PRIORITY", "RR" };
    int preemptives[] = { 0, 0, 1, 0, 1, 1 }; // 0: ������, 1: ������
    const char* names[] = { "FCFS", "SJF (NP)", "SJF (P)", "PRIORITY (NP)", "PRIORITY (P)", "RR" };

    printf("\n===== Algorithm Performance Evaluation =====\n"); // ===== �˰��� ���� �� =====
    for (int i = 0; i < 6; i++) {
        // ���μ��� ���� �� �ʱ�ȭ
        Process** copy = malloc(num_processes * sizeof(Process*));
        if (!copy) {
            fprintf(stderr, "Error: Failed to allocate memory for process copy.\n"); // ����: ���μ��� ���纻�� ���� �޸� �Ҵ翡 �����߽��ϴ�.
            return;
        }
        for (int j = 0; j < num_processes; j++) {
            copy[j] = malloc(sizeof(Process));
            if (!copy[j]) {
                fprintf(stderr, "Error: Failed to allocate memory for process copy[%d].\n", j); // ����: ���μ��� ���纻[%d]�� ���� �޸� �Ҵ翡 �����߽��ϴ�.
                // �̹� �Ҵ�� �޸� ����
                for (int k = 0; k < j; k++) free(copy[k]);
                free(copy);
                return;
            }
            memcpy(copy[j], original_processes[j], sizeof(Process));
            // ����� ���μ����� ���� �ʱ�ȭ
            copy[j]->remaining_cpu_time = copy[j]->cpu_burst_time;
            // I/O ���� �ʵ� �ʱ�ȭ ���ŵ�
            // copy[j]->io_index = 0;
            // copy[j]->io_remaining_time = 0;
            copy[j]->last_executed_time = -1;
            copy[j]->start_time = -1;
            copy[j]->completion_time = -1;
            copy[j]->state = NEW; // ����� ���μ����� NEW ���·� ����
        }

        float avg_tat = 0, avg_wt = 0;
        printf("\n[%s] Results:\n", names[i]); // [%s] ���:
        schedule(copy, num_processes, algorithms[i], preemptives[i], &avg_tat, &avg_wt);
        printf("Average Turnaround Time = %.2f, Average Waiting Time = %.2f\n", avg_tat, avg_wt); // ��� Turnaround Time = %.2f, ��� Waiting Time = %.2f

        for (int j = 0; j < num_processes; j++) free(copy[j]);
        free(copy);
    }
}

int main() {
    // 1. �ùķ����� ���� (Config() �Լ��� ���� �ڵ� ����)
    printf("===== CPU Scheduling Simulator Starting =====\n"); // ===== CPU �����ٸ� �ùķ����� ���� =====
    SystemConfig* sys_config = config(); // config ȣ���Ͽ� ���� �Ű����� �ڵ� ����
    if (!sys_config) {
        fprintf(stderr, "Error: Failed to initialize system configuration.\n"); // ����: �ý��� ���� �ʱ�ȭ�� �����߽��ϴ�.
        return 1;
    }
    // SystemConfig ����ü�� �� �̻� ������ ������ ����
    // ����: sys_config ������ ť�� schedule/evaluate���� ������ �����Ƿ�,
    // ���⼭ �������� ������ ��������� �޸� ������ �߻��մϴ�.
    // �� �ùķ��̼� ���������� �߿����� �ʽ��ϴ�.
    free(sys_config->ready_queue);
    // free(sys_config->waiting_queue); // ���ŵ�
    free(sys_config);


    // 2. ���μ��� ����
    srand(time(NULL)); // ���� ������ �õ� ���� (�� ����)
    Process** processes = (Process**)malloc(num_processes_global * sizeof(Process*)); // main �Լ� ������ processes ����
    if (!processes) {
        fprintf(stderr, "Error: Failed to allocate memory for process array.\n"); // ����: ���μ��� �迭 �޸� �Ҵ翡 �����߽��ϴ�.
        return 1;
    }

    printf("\n===== Generated Process Information =====\n"); // ===== ������ ���μ��� ���� =====
    for (int i = 0; i < num_processes_global; i++) {
        // create_process ȣ�� �� max_io_burst_global ���� ����
        processes[i] = create_process(i, max_arrival_time_global, max_cpu_burst_global, max_priority_global /* , max_io_burst_global ���ŵ� */);
        if (!processes[i]) {
            fprintf(stderr, "Error: Failed to create process P%d.\n", i); // ����: ���μ��� P%d ������ �����߽��ϴ�.
            // ������ �Ҵ�� �޸� ����
            for (int j = 0; j < i; j++) free(processes[j]);
            free(processes);
            return 1;
        }
        printf("PID: %d, Arrival Time: %d, CPU Burst: %d, Priority: %d\n", // I/O ���� ��� ����
            processes[i]->id, processes[i]->arrival_time, processes[i]->cpu_burst_time, processes[i]->priority);
        // I/O ��û ���� ��� ���� ����
        /*
        for (int k = 0; k < processes[i]->num_io_requests; k++) {
            printf("  I/O #%d: Burst %d, Request Time %d\n", k + 1, processes[i]->io_burst_time[k], processes[i]->io_request_time[k]);
        }
        */
    }

    // 3. �����ٸ� �˰��� ��
    evaluate(processes, num_processes_global);

    // 4. �޸� ����
    for (int i = 0; i < num_processes_global; i++) {
        free(processes[i]);
    }
    free(processes);

    printf("\nCPU Scheduling Simulator finished.\n"); // CPU �����ٸ� �ùķ����� ����.

    return 0;
}