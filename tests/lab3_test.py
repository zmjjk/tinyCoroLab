import sys
import subprocess
import time

paras = [(1, 8000), (0, 8001)]


def run_test(
    thread_num: int,
    server_port: int,
    benchprogram_path: str,
    benchtool_path: str,
) -> bool:
    try:
        program_process = subprocess.Popen(
            [benchprogram_path, str(thread_num), str(server_port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        time.sleep(2)
        if program_process.poll() is not None:  # terminate
            stdout, stderr = program_process.communicate()
            print(f"[error] {benchprogram_path} start failed")
            print(f"[return code] {program_process.returncode}")
            print(f"[stdout] {stdout}")
            print(f"[stderr] {stderr}")
            return False

        bench_process = subprocess.Popen(
            [
                benchtool_path,
                "--address",
                f"127.0.0.1:8002",
                "--number",
                "100",
                "--duration",
                "30",
                "--length",
                "1024",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        time.sleep(2)
        if bench_process.stderr.readline() != "":
            print("[error] bench tools running error")
            bench_process.kill()
            _, stderr = bench_process.communicate()
            print(f"[stderr] {stderr}")
            if program_process.poll() is None:
                program_process.kill()
            return False

        time.sleep(40)

        # check status
        if bench_process.poll() is None:  # running
            stdout, stderr = program_process.communicate()
            print(
                f"[error] {benchtool_path} hasn't finish running, please check your server code"
            )
            bench_process.kill()
            if program_process.poll() is None:
                program_process.kill()
            return False

        if program_process.poll() is not None:  # terminate
            stdout, stderr = program_process.communicate()
            print(f"[error] {benchprogram_path} crashed when test")
            print(f"[return code] {program_process.returncode}")
            print(f"[stdout] {stdout}")
            print(f"[stderr] {stderr}")
            return False

        # collect result
        stdout, _ = bench_process.communicate()
        print(f"[test result] {stdout}")

        program_process.kill()

    except Exception as e:
        print(f"exception occur! detail: {e}")
        return False

    return True


if __name__ == "__main__":
    for i, para in enumerate(paras):
        print(f"================== lab3 test case {i} begin ==================")
        thread_num = para[0]
        if thread_num == 0:
            thread_num = "default core number"
        print(f"thread number: {thread_num}, server port: {para[1]}")
        if not run_test(para[0], para[1], sys.argv[1], sys.argv[2]):
            print(f"xxxxxxxxxxxxxxxxxx lab3 test case {i} error xxxxxxxxxxxxxxxxxx")
            sys.exit(1)
        else:
            print(f"================== lab3 test case {i} finish =================")
            time.sleep(2)
    print("you pass lab3 test, congratulations!")
