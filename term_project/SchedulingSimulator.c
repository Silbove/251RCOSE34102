#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define INITIAL_READY_QUEUE_CAPACITY 10  // 초기 준비 큐 용량
#define INITIAL_WAITING_QUEUE_CAPACITY 5 // 초기 대기 큐 용량
#define MAX_IO_REQUESTS 3                // 최대 I/O 요청 수
#define TIME_QUANTUM 3                   // 라운드 로빈 타임 퀀텀

// 프로세스 상태 열거형
typedef enum {
    NEW,        // 생성됨
    READY,      // 실행 준비됨
    RUNNING,    // 실행 중
    WAITING,    // I/O 대기 중
    TERMINATED  // 종료됨
} ProcessState;

// 프로세스 구조체 (다중 I/O 요청 지원)
typedef struct {
    int id;                             // 프로세스 ID
    int arrival_time;                   // 도착 시간
    int cpu_burst_time;                 // 총 CPU 버스트 시간
    int priority;                       // 우선순위 (숫자가 클수록 높은 우선순위)

    int num_io_requests;                // I/O 요청 횟수
    int io_burst_time[MAX_IO_REQUESTS]; // 각 I/O의 버스트 시간
    int io_request_time[MAX_IO_REQUESTS]; // 각 I/O 요청 시점 (CPU 사용 시간 기준)

    int remaining_cpu_time;             // 남은 CPU 시간
    int io_index;                       // 현재 I/O 요청 인덱스
    int io_remaining_time;             // 현재 I/O 남은 시간
    int last_executed_time;            // 마지막 실행 시간 (선점형 고려)
    int start_time;                    // 첫 실행 시간
    int completion_time;               // 완료 시간

    ProcessState state;                 // 현재 프로세스 상태
} Process;

// 시스템 환경 구조체
typedef struct {
    Process** ready_queue;
    int ready_queue_size;
    int ready_queue_capacity;

    Process** waiting_queue;
    int waiting_queue_size;
    int waiting_queue_capacity;
} SystemConfig;

// 프로세스 생성 함수
Process* create_process(int id, int max_arrival_time, int max_cpu_burst, int max_priority, int max_io_burst) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    if (new_process == NULL) {
        perror("프로세스 메모리 할당 실패");
        return NULL;
    }

    new_process->id = id;
    new_process->arrival_time = rand() % (max_arrival_time + 1);
    new_process->cpu_burst_time = (rand() % max_cpu_burst) + 1;
    new_process->priority = rand() % (max_priority + 1);
    new_process->state = NEW;

    new_process->remaining_cpu_time = new_process->cpu_burst_time;
    new_process->io_index = 0;
    new_process->io_remaining_time = 0;
    new_process->last_executed_time = -1;
    new_process->start_time = -1;
    new_process->completion_time = -1;

    new_process->num_io_requests = rand() % (MAX_IO_REQUESTS + 1);
    for (int i = 0; i < new_process->num_io_requests; i++) {
        new_process->io_burst_time[i] = (rand() % max_io_burst) + 1;
        int request_time;
        do {
            request_time = rand() % new_process->cpu_burst_time;
        } while (request_time == 0 || request_time >= new_process->cpu_burst_time);
        new_process->io_request_time[i] = request_time;
    }

    return new_process;
}

// 시스템 환경 설정 함수
SystemConfig* config() {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    if (config == NULL) {
        perror("시스템 설정 메모리 할당 실패");
        return NULL;
    }

    config->ready_queue_capacity = INITIAL_READY_QUEUE_CAPACITY;
    config->ready_queue_size = 0;
    config->ready_queue = (Process**)malloc(config->ready_queue_capacity * sizeof(Process*));
    if (config->ready_queue == NULL) {
        perror("준비 큐 메모리 할당 실패");
        free(config);
        return NULL;
    }

    config->waiting_queue_capacity = INITIAL_WAITING_QUEUE_CAPACITY;
    config->waiting_queue_size = 0;
    config->waiting_queue = (Process**)malloc(config->waiting_queue_capacity * sizeof(Process*));
    if (config->waiting_queue == NULL) {
        perror("대기 큐 메모리 할당 실패");
        free(config->ready_queue);
        free(config);
        return NULL;
    }

    srand(time(NULL));

    printf("시스템 환경 설정 완료.\n");
    printf("  - 초기 준비 큐 용량: %d\n", config->ready_queue_capacity);
    printf("  - 초기 대기 큐 용량: %d\n", config->waiting_queue_capacity);

    return config;
}

// Gantt Chart 출력을 위한 구조체 정의
typedef struct {
    int time;
    int pid;
} GanttUnit;

// schedule 함수 선언 (구현은 이후에 계속 작성)
void schedule(Process** processes, int num_processes, SystemConfig* system, const char* algorithm, int preemptive);

// 예시 사용법 (main 함수는 포함하지 않음, 함수 사용 예시)
/*
int main() {
    // 시스템 환경 설정 (#define으로 정의된 용량 사용)
    SystemConfig *system_config = config();

    if (system_config == NULL) {
        return 1; // 설정 실패 시 종료
    }

    // 프로세스 생성 (예시: 5개 프로세스 생성)
    int num_processes_to_create = 5;
    Process *processes[num_processes_to_create]; // 생성된 프로세스 포인터를 저장할 배열

    printf("\n프로세스 생성:\n");
    for (int i = 0; i < num_processes_to_create; i++) {
        // create_process 함수 호출 (랜덤 값 범위는 적절히 조절)
        processes[i] = create_process(i + 1, 20, 10, 5, 7, 5);
        if (processes[i] != NULL) {
            printf("  - 프로세스 %d 생성: 도착 시간 %d, CPU 버스트 %d, 우선순위 %d, I/O 버스트 %d, I/O 요청 시간 %d\n",
                   processes[i]->id,
                   processes[i]->arrival_time,
                   processes[i]->cpu_burst_time,
                   processes[i]->priority,
                   processes[i]->io_burst_time,
                   processes[i]->io_request_time);
        }
    }

    // 생성된 프로세스 및 시스템 설정 메모리 해제 (실제 시뮬레이션 종료 시점에 수행)
    for (int i = 0; i < num_processes_to_create; i++) {
        free(processes[i]);
    }
    // 큐에 추가된 프로세스가 있다면 해당 프로세스 메모리도 해제해야 함 (여기서는 예시이므로 생략)
    free(system_config->ready_queue);
    free(system_config->waiting_queue);
    free(system_config);

    return 0;
}
*/
