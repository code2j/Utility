from multiprocessing import Process, Manager
import time



# 서로 다른 작업 함수들
def task_a(shared):
    while True:
        shared['a_count'] += 1
        time.sleep(1)

def task_b(shared):
    while True:
        shared['b_status'] = f"작동 중 ({time.time():.0f})"
        time.sleep(2)


if __name__ == "__main__":
    manager = Manager()
    shared = manager.dict(
        {
            'a_count': 0,
            'b_status': "대기"
        })

    # 프로세스 생성
    workers = {}
    workers["counter_worker"] = Process(target=task_a, args=(shared,))
    workers["status_worker"]  = Process(target=task_b, args=(shared,))


    # 시작
    for name, p in workers.items():
        print(f"[{name}] 시작 중...")
        p.init()

    # 메인 로직 (데이터 확인)
    try:
        for _ in range(50):
            print(f"실시간 데이터: {dict(shared)}")
            time.sleep(0.5)
    except KeyboardInterrupt:
        pass

    # 반복문으로 일괄 종료
    print("\n[시스템 종료]")
    for name, p in workers.items():
        p.terminate()
        p.join()
        print(f"[{name}] 안전하게 종료됨.")

    manager.shutdown()