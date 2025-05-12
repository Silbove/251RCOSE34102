#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define INITIAL_READY_QUEUE_CAPACITY 10
#define INITIAL_WAITING_QUEUE_CAPACITY 5

// 프로세스 상태 열거형
typedef enum {
    NEW,        // 생성됨
    READY,      // 실행 준비 완료
    RUNNING,    // 실행 중
    WAITING,    // I/O 대기 중
    TERMINATED  // 종료됨
} ProcessState;

// 프로세스 정보를 저장할 구조체
typedef struct {
    int id;             // 프로세스 ID
    int arrival_time;   // 도착 시간
    int cpu_burst_time; // CPU 버스트 시간
    int priority;       // 우선순위 (낮은 숫자가 높은 우선순위)
    int io_burst_time;  // I/O 버스트 시간 (0이면 I/O 없음)
    int io_request_time; // I/O 요청 시간 (CPU 버스트 중 I/O가 발생하는 시점)
    ProcessState state; // 현재 상태
    // 실제 시뮬레이션에 필요한 다른 필드 추가 가능 (예: 남은 CPU 버스트 시간, 남은 I/O 버스트 시간 등)
} Process;

// 시스템 환경 설정을 위한 구조체
typedef struct {
    Process** ready_queue;  // 준비 큐 (READY 상태 프로세스)
    int ready_queue_size;
    int ready_queue_capacity;

    Process** waiting_queue; // 대기 큐 (WAITING 상태 프로세스)
    int waiting_queue_size;
    int waiting_queue_capacity;

    // 기타 시스템 관련 설정 추가 가능 (예: CPU 코어 개수 등)
} SystemConfig;

// 프로세스 생성 함수
// num_processes: 생성할 프로세스 개수
Process* create_process(int id, int max_arrival_time, int max_cpu_burst, int max_priority, int max_io_burst, int max_io_request_offset) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    if (new_process == NULL) {
        perror("프로세스 메모리 할당 실패");
        return NULL;
    }

    new_process->id = id;
    // 도착 시간: 0부터 max_arrival_time 사이의 랜덤 값
    new_process->arrival_time = rand() % (max_arrival_time + 1);
    // CPU 버스트 시간: 1부터 max_cpu_burst 사이의 랜덤 값
    new_process->cpu_burst_time = (rand() % max_cpu_burst) + 1;
    // 우선순위: 0부터 max_priority 사이의 랜덤 값 (낮은 숫자가 높은 우선순위)
    new_process->priority = rand() % (max_priority + 1);

    // I/O 발생 여부 및 시간 랜덤 설정 (예: 30% 확률로 I/O 발생)
    if (rand() % 100 < 30) {
        // I/O 버스트 시간: 1부터 max_io_burst 사이의 랜덤 값
        new_process->io_burst_time = (rand() % max_io_burst) + 1;
        // I/O 요청 시간: CPU 버스트 시간 내에서 랜덤하게 설정
        // I/O 요청 시간은 0보다 크고 CPU 버스트 시간보다 작아야 함
        if (new_process->cpu_burst_time > 1) {
            new_process->io_request_time = rand() % (new_process->cpu_burst_time - 1) + 1;
        }
        else {
            // CPU 버스트 시간이 1이면 I/O 발생 어려움, 0으로 설정
            new_process->io_burst_time = 0;
            new_process->io_request_time = 0;
        }

    }
    else {
        new_process->io_burst_time = 0;
        new_process->io_request_time = 0;
    }

    new_process->state = NEW; // 초기 상태는 NEW

    return new_process;
}

// 시스템 환경 설정 함수 (큐 용량은 #define으로 정의)
SystemConfig* config() {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    if (config == NULL) {
        perror("시스템 설정 메모리 할당 실패");
        return NULL;
    }

    // 준비 큐 초기화 (#define으로 정의된 용량 사용)
    config->ready_queue_capacity = INITIAL_READY_QUEUE_CAPACITY;
    config->ready_queue_size = 0;
    config->ready_queue = (Process**)malloc(config->ready_queue_capacity * sizeof(Process*));
    if (config->ready_queue == NULL) {
        perror("준비 큐 메모리 할당 실패");
        free(config); // config 구조체 메모리 해제
        return NULL;
    }

    // 대기 큐 초기화 (#define으로 정의된 용량 사용)
    config->waiting_queue_capacity = INITIAL_WAITING_QUEUE_CAPACITY;
    config->waiting_queue_size = 0;
    config->waiting_queue = (Process**)malloc(config->waiting_queue_capacity * sizeof(Process*));
    if (config->waiting_queue == NULL) {
        perror("대기 큐 메모리 할당 실패");
        free(config->ready_queue); // 준비 큐 메모리 해제
        free(config); // config 구조체 메모리 해제
        return NULL;
    }

    // 랜덤 시드 초기화 (시간 기반)
    srand(time(NULL));

    printf("시스템 환경 설정 완료.\n");
    printf("  - 초기 준비 큐 용량: %d\n", config->ready_queue_capacity);
    printf("  - 초기 대기 큐 용량: %d\n", config->waiting_queue_capacity);

    return config;
}

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
