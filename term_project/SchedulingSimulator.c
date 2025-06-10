#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define INITIAL_READY_QUEUE_CAPACITY 10 // 초기 준비 큐 용량
#define INITIAL_WAITING_QUEUE_CAPACITY 5 // 초기 대기 큐 용량 (더 이상 사용되지 않음)
// #define MAX_IO_REQUESTS 3 // 프로세스당 최대 I/O 요청 수 (제거됨)
#define TIME_QUANTUM 3 // 라운드 로빈 시간 할당량

// 프로세스 상태 정의
typedef enum {
    NEW, // 새로 생성됨
    READY, // 실행 준비 완료
    RUNNING, // CPU에서 실행 중
    // WAITING, // I/O 대기 중 (제거됨)
    TERMINATED // 실행 완료
} ProcessState;

// 프로세스 구조체 정의
typedef struct {
    int id; // 프로세스 ID
    int arrival_time; // 도착 시간
    int cpu_burst_time; // 총 CPU 버스트 시간
    int priority; // 우선순위

    // I/O 관련 필드 제거됨
    // int num_io_requests; 
    // int io_burst_time[MAX_IO_REQUESTS];
    // int io_request_time[MAX_IO_REQUESTS];

    int remaining_cpu_time; // 남은 CPU 실행 시간
    // int io_index; // 현재 처리 중인 I/O 요청 인덱스 (제거됨)
    // int io_remaining_time; // 남은 I/O 시간 (제거됨)
    int last_executed_time; // 마지막으로 CPU를 할당받은 시간 (현재 코드에서는 사용되지 않음)
    int start_time; // 프로세스가 처음 CPU를 할당받은 시간
    int completion_time; // 완료 시간

    ProcessState state; // 현재 프로세스 상태
} Process;

// 시스템 큐 구성 구조체 (현재 schedule 함수에서 직접 사용되지 않음)
typedef struct {
    Process** ready_queue; // 준비 큐 (프로세스 포인터 배열)
    int ready_queue_size; // 준비 큐 현재 크기
    int ready_queue_capacity; // 준비 큐 용량

    // Waiting Queue 관련 필드 제거됨
    // Process** waiting_queue; 
    // int waiting_queue_size;
    // int waiting_queue_capacity;
} SystemConfig;

// 전역 변수 (config() 함수에 의해 설정될 시뮬레이션 매개변수)
int num_processes_global = 0; // 전체 프로세스 수
int max_arrival_time_global = 0; // 최대 도착 시간
int max_cpu_burst_global = 0; // 최대 CPU 버스트 시간
int max_priority_global = 0; // 최대 우선순위
// int max_io_burst_global = 0; // 최대 I/O 버스트 시간 (제거됨)


// 새로운 프로세스 생성
Process* create_process(int id, int max_arrival_time, int max_cpu_burst, int max_priority /*, int max_io_burst 제거됨 */) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    if (!new_process) return NULL; // 메모리 할당 실패 시 NULL 반환

    new_process->id = id;
    new_process->arrival_time = rand() % (max_arrival_time + 1); // 0부터 max_arrival_time까지 랜덤 생성
    new_process->cpu_burst_time = (rand() % max_cpu_burst) + 1; // 1부터 max_cpu_burst까지 랜덤 생성
    new_process->priority = rand() % (max_priority + 1); // 0부터 max_priority까지 랜덤 생성 (낮은 숫자가 높은 우선순위)
    new_process->state = NEW; // 초기 상태는 NEW

    new_process->remaining_cpu_time = new_process->cpu_burst_time; // 남은 CPU 시간 초기화
    // I/O 관련 필드 초기화 제거됨
    // new_process->io_index = 0; 
    // new_process->io_remaining_time = 0; 
    new_process->last_executed_time = -1; // 마지막 실행 시간 초기화
    new_process->start_time = -1; // 시작 시간 초기화
    new_process->completion_time = -1; // 완료 시간 초기화

    // I/O 요청 생성 로직 제거됨
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

// 시스템 설정 함수 (이제 전역 매개변수를 자동으로 설정)
SystemConfig* config() {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    if (!config) return NULL; // 메모리 할당 실패 시 NULL 반환

    config->ready_queue_capacity = INITIAL_READY_QUEUE_CAPACITY; // 준비 큐 초기 용량 설정
    config->ready_queue_size = 0; // 준비 큐 크기 초기화
    config->ready_queue = (Process**)malloc(config->ready_queue_capacity * sizeof(Process*)); // 준비 큐 메모리 할당

    // Waiting Queue 관련 필드 초기화 제거됨
    // config->waiting_queue_capacity = INITIAL_WAITING_QUEUE_CAPACITY;
    // config->waiting_queue_size = 0;
    // config->waiting_queue = (Process**)malloc(config->waiting_queue_capacity * sizeof(Process*));

    // 전역 시뮬레이션 매개변수를 자동으로 설정
    num_processes_global = 5;      // 기본 프로세스 수
    max_arrival_time_global = 15;   // 기본 최대 도착 시간
    max_cpu_burst_global = 25;      // 기본 최대 CPU 버스트 시간
    max_priority_global = 5;        // 기본 최대 우선순위 (0-5)
    // max_io_burst_global = 10;       // 기본 최대 I/O 버스트 시간 (제거됨)

    printf("\nSystem environment configuration automatically set:\n");
    printf("  Number of Processes: %d\n", num_processes_global);
    printf("  Max Arrival Time: %d\n", max_arrival_time_global);
    printf("  Max CPU Burst Time: %d\n", max_cpu_burst_global);
    printf("  Max Priority: %d\n", max_priority_global);
    // printf("  Max I/O Burst Time: %d\n", max_io_burst_global); // 제거됨
    return config;
}

void print_gantt_chart_formatted(int* gantt_array, int total_time_units) {
    printf("\n  Formatted Gantt Chart:\n");
    if (total_time_units == 0) {
        printf("    (No execution recorded)\n"); // 실행 기록 없음
        return;
    }

    // "    CPU : "의 길이 (공백 포함)
    const int LABEL_PREFIX_WIDTH = 8;

    // CPU 라인 버퍼 (최대 가능한 길이로 할당)
    // 각 블록 "P#(duration)" 또는 "Idle(duration)"은 최대 20자 + 구분자 '-' 1자
    // 총 시간 단위 * (최대 블록 길이 + 구분자) + 여유분
    char* cpu_line_buffer = (char*)malloc(total_time_units * 21 + 100);
    if (!cpu_line_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for cpu_line_buffer.\n");
        return;
    }
    cpu_line_buffer[0] = '\0'; // 버퍼 초기화

    int current_cpu_char_pos = 0; // cpu_line_buffer 내 현재 문자 위치 (접두사 제외)

    int last_pid = -2; // 이전 프로세스 ID (초기값은 유효하지 않은 PID)

    // CPU 라인 버퍼를 채우고 각 시간 단위의 문자 위치 기록
    for (int t = 0; t < total_time_units; t++) {
        // 현재 시간 단위의 PID
        int current_pid_at_t = gantt_array[t];

        // 새로운 블록이 시작되는 경우 (이전 PID와 다르거나 첫 번째 시간 단위인 경우)
        if (current_pid_at_t != last_pid || t == 0) {
            if (t > 0) { // 첫 블록이 아니면 구분자 추가
                strcat(cpu_line_buffer, "-");
                current_cpu_char_pos++;
            }

            last_pid = current_pid_at_t;

            // 현재 블록의 지속 시간 계산
            int block_duration = 0;
            for (int k = t; k < total_time_units && gantt_array[k] == last_pid; k++) {
                block_duration++;
            }

            char block_str[20]; // "P#(duration)" 또는 "Idle(duration)" 문자열
            int block_len;
            if (last_pid != -1) { // 프로세스 실행 블록
                block_len = snprintf(block_str, sizeof(block_str), "P%d(%d)", last_pid, block_duration);
            }
            else { // 유휴 블록
                block_len = snprintf(block_str, sizeof(block_str), "Idle(%d)", block_duration);
            }

            strcat(cpu_line_buffer, block_str);
            current_cpu_char_pos += block_len;

            // 현재 t를 블록의 끝으로 이동하여 다음 반복에서 새 블록을 시작하도록 함
            // (이미 처리한 시간 단위는 건너뛰기 위함)
            t += block_duration - 1;
        }
    }
    cpu_line_buffer[current_cpu_char_pos] = '\0';

    // 포맷된 간트 차트 출력
    // Time 라인 출력 코드를 제거했습니다.
    printf("    CPU : %s\n", cpu_line_buffer);

    // 할당된 메모리 해제
    free(cpu_line_buffer);
}


// 스케줄링 함수
void schedule(Process** processes_arr, int num_processes_arr, const char* algorithm, int preemptive, float* avg_tat, float* avg_wt) {
    int time = 0, completed = 0;
    long total_turnaround_time = 0, total_waiting_time = 0; // 오버플로우 방지를 위해 long 사용
    int gantt_capacity = 1000; // 초기 간트 차트 용량
    int gantt_size = 0; // 기록된 실제 시간 단위 수
    int* gantt_pid = malloc(gantt_capacity * sizeof(int)); // 각 시간 단위에 실행 중인 프로세스 PID를 저장할 배열

    if (!gantt_pid) {
        fprintf(stderr, "Error: Failed to allocate memory for gantt_pid.\n"); // 오류: gantt_pid 메모리 할당에 실패했습니다.
        return;
    }

    // 각 시뮬레이션 시작 전 프로세스 상태 초기화
    for (int i = 0; i < num_processes_arr; i++) {
        processes_arr[i]->remaining_cpu_time = processes_arr[i]->cpu_burst_time;
        // I/O 관련 초기화 제거됨
        // processes_arr[i]->io_index = 0;
        // processes_arr[i]->io_remaining_time = 0;
        processes_arr[i]->last_executed_time = -1;
        processes_arr[i]->start_time = -1;
        processes_arr[i]->completion_time = -1;
        processes_arr[i]->state = NEW; // 모든 프로세스를 NEW 상태로 초기화
    }

    // Round Robin을 위한 정적 변수 (함수 호출 간에 상태 유지를 위함)
    static int rr_index_static = 0; // 이 변수는 함수 호출 간에 값을 유지해야 합니다.
    if (strcmp(algorithm, "RR") == 0) {
        rr_index_static = 0; // RR 시뮬레이션 시작 시 인덱스 초기화
    }


    while (completed < num_processes_arr) {
        // I/O 처리 및 WAITING -> READY 전환 로직 제거됨
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

        // NEW -> READY 전환 (도착 시간 기준)
        for (int i = 0; i < num_processes_arr; i++) {
            Process* p = processes_arr[i];
            if (p->state == NEW && p->arrival_time <= time) {
                p->state = READY;
            }
        }

        // 실행할 프로세스 선택
        Process* selected = NULL;

        // Ready 상태의 프로세스들 중에서 선택 (RR 제외)
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
                            // 낮은 우선순위 숫자가 높은 우선순위라고 가정
                            if (p->priority < selected->priority) {
                                selected = p;
                            }
                        }
                    }
                }
            }
        }
        else { // 라운드 로빈 알고리즘 처리 (선택 로직)
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
            } while (rr_index_static != initial_rr_index); // 모든 프로세스를 한 바퀴 돌았는지 확인

            if (!found_rr) { // Ready Queue에 실행할 프로세스가 없으면
                time++;
                // 간트 차트용 유휴 시간 기록
                if (gantt_size >= gantt_capacity) { // 필요시 재할당
                    gantt_capacity *= 2;
                    gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                    if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // 오류: 재할당 실패.
                }
                gantt_pid[gantt_size++] = -1; // -1은 유휴 상태를 나타냄
                if (time > gantt_capacity * num_processes_arr * 2) { // 안전 장치
                    fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n"); // 경고: 시뮬레이션 시간이 안전 한도를 초과했습니다. 중단합니다.
                    break;
                }
                continue; // 다음 시간 단위로 이동
            }
        }


        if (selected == NULL) { // 실행할 프로세스가 없으면 (모두 TERMINATED 상태)
            time++;
            // 간트 차트용 유휴 시간 기록
            if (gantt_size >= gantt_capacity) { // 필요시 재할당
                gantt_capacity *= 2;
                gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // 오류: 재할당 실패.
            }
            gantt_pid[gantt_size++] = -1; // -1은 유휴 상태를 나타냄
            if (time > gantt_capacity * num_processes_arr * 2) { // 안전 장치
                fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n"); // 경고: 시뮬레이션 시간이 안전 한도를 초과했습니다. 중단합니다.
                break;
            }
            continue; // 다음 시간 단위로 이동
        }

        if (selected->start_time == -1) {
            selected->start_time = time;
        }
        selected->state = RUNNING;


        int execute_duration = 1; // 기본 실행 시간은 1 시간 단위
        if (strcmp(algorithm, "RR") == 0) {
            execute_duration = (selected->remaining_cpu_time < TIME_QUANTUM) ? selected->remaining_cpu_time : TIME_QUANTUM;
            // RR의 경우, 다음 프로세스의 차례를 미리 결정
            rr_index_static = (rr_index_static + 1) % num_processes_arr;
        }
        else if (!preemptive) {
            // 비선점형 알고리즘은 한 번 선택되면 완료될 때까지 실행 (I/O 없으므로 남은 시간 모두 실행)
            execute_duration = selected->remaining_cpu_time;
            // I/O 요청 관련 로직 제거됨
        }

        // 프로세스 실행 (execute_duration 만큼)
        for (int t_unit = 0; t_unit < execute_duration; t_unit++) {
            if (gantt_size >= gantt_capacity) { // 필요시 재할당
                gantt_capacity *= 2;
                gantt_pid = realloc(gantt_pid, gantt_capacity * sizeof(int));
                if (!gantt_pid) { fprintf(stderr, "Error: Reallocation failed.\n"); return; } // 오류: 재할당 실패.
            }
            gantt_pid[gantt_size++] = selected->id; // 간트 차트용 실행 프로세스 기록

            selected->remaining_cpu_time--;
            time++;

            // I/O 요청 처리 로직 제거됨
            /*
            if (selected->io_index < selected->num_io_requests &&
                (selected->cpu_burst_time - selected->remaining_cpu_time) == selected->io_request_time[selected->io_index]) {
                selected->io_remaining_time = selected->io_burst_time[selected->io_index];
                selected->state = WAITING;
                selected->io_index++;
                break; // I/O 요청 발생 시 현재 실행 중단
            }
            */

            // 프로세스 종료
            if (selected->remaining_cpu_time == 0) {
                selected->completion_time = time;
                selected->state = TERMINATED;
                completed++;
                break; // 프로세스 완료 시 현재 실행 중단
            }

            // 선점형 알고리즘에서 선점 확인
            if (preemptive && (strcmp(algorithm, "SJF") == 0 || strcmp(algorithm, "PRIORITY") == 0)) {
                Process* potential_preemptor = NULL;
                for (int j = 0; j < num_processes_arr; j++) {
                    Process* p_check = processes_arr[j];
                    if (p_check->state == NEW && p_check->arrival_time <= time) { // 새로 도착한 프로세스 READY로 전환
                        p_check->state = READY;
                    }
                    if (p_check->state == READY) {
                        if (potential_preemptor == NULL) {
                            potential_preemptor = p_check;
                        }
                        else {
                            if (strcmp(algorithm, "SJF") == 0) {
                                if (p_check->remaining_cpu_time < selected->remaining_cpu_time) { // selected와 비교
                                    potential_preemptor = p_check;
                                }
                            }
                            else if (strcmp(algorithm, "PRIORITY") == 0) {
                                // 낮은 우선순위 숫자가 높은 우선순위라고 가정
                                if (p_check->priority < selected->priority) { // selected와 비교
                                    potential_preemptor = p_check;
                                }
                            }
                        }
                    }
                }

                if (potential_preemptor != NULL && potential_preemptor->id != selected->id) {
                    if ((strcmp(algorithm, "SJF") == 0 && potential_preemptor->remaining_cpu_time < selected->remaining_cpu_time) ||
                        (strcmp(algorithm, "PRIORITY") == 0 && potential_preemptor->priority < selected->priority)) {
                        selected->state = READY; // 현재 프로세스를 READY로 되돌림
                        goto preempt_break; // 내부 루프를 빠져나가 다음 스케줄링 사이클로 이동
                    }
                }
            }
        }
    preempt_break:; // 선점 발생 시 점프 지점

        // 현재 선택된 프로세스가 완료되지 않았고 I/O 대기 상태도 아니면 다시 READY로
        // I/O 대기 상태가 없으므로 조건이 간소화됨
        if (selected != NULL && selected->state == RUNNING) {
            selected->state = READY; // 현재 실행 중인 프로세스를 Ready로 되돌림
        }

        if (time > gantt_capacity * num_processes_arr * 2) { // 안전 장치: 과도하게 긴 시뮬레이션 방지
            fprintf(stderr, "Warning: Simulation time exceeded safety limit. Breaking.\n");
            break;
        }
    }

    // TAT, WT 계산
    int actual_completed_count = 0;
    for (int i = 0; i < num_processes_arr; i++) {
        if (processes_arr[i]->state == TERMINATED) {
            int tat = processes_arr[i]->completion_time - processes_arr[i]->arrival_time;
            // WT = TAT - 총 CPU 버스트 시간 (I/O 시간은 고려하지 않음)
            int wt = tat - processes_arr[i]->cpu_burst_time;

            if (wt < 0) wt = 0; // 대기 시간은 음수가 될 수 없음

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


    // 새로운 형식화된 함수를 사용하여 간트 차트 출력
    print_gantt_chart_formatted(gantt_pid, gantt_size);

    free(gantt_pid);
}

// 평가 함수
void evaluate(Process** original_processes, int num_processes) {
    const char* algorithms[] = { "FCFS", "SJF", "SJF", "PRIORITY", "PRIORITY", "RR" };
    int preemptives[] = { 0, 0, 1, 0, 1, 1 }; // 0: 비선점형, 1: 선점형
    const char* names[] = { "FCFS", "SJF (NP)", "SJF (P)", "PRIORITY (NP)", "PRIORITY (P)", "RR" };

    printf("\n===== Algorithm Performance Evaluation =====\n"); // ===== 알고리즘 성능 평가 =====
    for (int i = 0; i < 6; i++) {
        // 프로세스 복사 및 초기화
        Process** copy = malloc(num_processes * sizeof(Process*));
        if (!copy) {
            fprintf(stderr, "Error: Failed to allocate memory for process copy.\n"); // 오류: 프로세스 복사본을 위한 메모리 할당에 실패했습니다.
            return;
        }
        for (int j = 0; j < num_processes; j++) {
            copy[j] = malloc(sizeof(Process));
            if (!copy[j]) {
                fprintf(stderr, "Error: Failed to allocate memory for process copy[%d].\n", j); // 오류: 프로세스 복사본[%d]을 위한 메모리 할당에 실패했습니다.
                // 이미 할당된 메모리 정리
                for (int k = 0; k < j; k++) free(copy[k]);
                free(copy);
                return;
            }
            memcpy(copy[j], original_processes[j], sizeof(Process));
            // 복사된 프로세스의 상태 초기화
            copy[j]->remaining_cpu_time = copy[j]->cpu_burst_time;
            // I/O 관련 필드 초기화 제거됨
            // copy[j]->io_index = 0;
            // copy[j]->io_remaining_time = 0;
            copy[j]->last_executed_time = -1;
            copy[j]->start_time = -1;
            copy[j]->completion_time = -1;
            copy[j]->state = NEW; // 복사된 프로세스도 NEW 상태로 시작
        }

        float avg_tat = 0, avg_wt = 0;
        printf("\n[%s] Results:\n", names[i]); // [%s] 결과:
        schedule(copy, num_processes, algorithms[i], preemptives[i], &avg_tat, &avg_wt);
        printf("Average Turnaround Time = %.2f, Average Waiting Time = %.2f\n", avg_tat, avg_wt); // 평균 Turnaround Time = %.2f, 평균 Waiting Time = %.2f

        for (int j = 0; j < num_processes; j++) free(copy[j]);
        free(copy);
    }
}

int main() {
    // 1. 시뮬레이터 설정 (Config() 함수를 통한 자동 설정)
    printf("===== CPU Scheduling Simulator Starting =====\n"); // ===== CPU 스케줄링 시뮬레이터 시작 =====
    SystemConfig* sys_config = config(); // config 호출하여 전역 매개변수 자동 설정
    if (!sys_config) {
        fprintf(stderr, "Error: Failed to initialize system configuration.\n"); // 오류: 시스템 설정 초기화에 실패했습니다.
        return 1;
    }
    // SystemConfig 구조체가 더 이상 사용되지 않으면 해제
    // 참고: sys_config 내부의 큐는 schedule/evaluate에서 사용되지 않으므로,
    // 여기서 해제하지 않으면 기술적으로 메모리 누수가 발생합니다.
    // 이 시뮬레이션 로직에서는 중요하지 않습니다.
    free(sys_config->ready_queue);
    // free(sys_config->waiting_queue); // 제거됨
    free(sys_config);


    // 2. 프로세스 생성
    srand(time(NULL)); // 난수 생성기 시드 설정 (한 번만)
    Process** processes = (Process**)malloc(num_processes_global * sizeof(Process*)); // main 함수 내에서 processes 선언
    if (!processes) {
        fprintf(stderr, "Error: Failed to allocate memory for process array.\n"); // 오류: 프로세스 배열 메모리 할당에 실패했습니다.
        return 1;
    }

    printf("\n===== Generated Process Information =====\n"); // ===== 생성된 프로세스 정보 =====
    for (int i = 0; i < num_processes_global; i++) {
        // create_process 호출 시 max_io_burst_global 인자 제거
        processes[i] = create_process(i, max_arrival_time_global, max_cpu_burst_global, max_priority_global /* , max_io_burst_global 제거됨 */);
        if (!processes[i]) {
            fprintf(stderr, "Error: Failed to create process P%d.\n", i); // 오류: 프로세스 P%d 생성에 실패했습니다.
            // 이전에 할당된 메모리 해제
            for (int j = 0; j < i; j++) free(processes[j]);
            free(processes);
            return 1;
        }
        printf("PID: %d, Arrival Time: %d, CPU Burst: %d, Priority: %d\n", // I/O 정보 출력 제거
            processes[i]->id, processes[i]->arrival_time, processes[i]->cpu_burst_time, processes[i]->priority);
        // I/O 요청 정보 출력 루프 제거
        /*
        for (int k = 0; k < processes[i]->num_io_requests; k++) {
            printf("  I/O #%d: Burst %d, Request Time %d\n", k + 1, processes[i]->io_burst_time[k], processes[i]->io_request_time[k]);
        }
        */
    }

    // 3. 스케줄링 알고리즘 평가
    evaluate(processes, num_processes_global);

    // 4. 메모리 해제
    for (int i = 0; i < num_processes_global; i++) {
        free(processes[i]);
    }
    free(processes);

    printf("\nCPU Scheduling Simulator finished.\n"); // CPU 스케줄링 시뮬레이터 종료.

    return 0;
}