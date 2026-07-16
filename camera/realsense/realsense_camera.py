import pyrealsense2 as rs
import numpy as np
import cv2

class RealSenseCamera:
    def __init__(self, width=848, height=480, fps=30):
        self.width    = width
        self.height   = height
        self.fps      = fps

        self.pipeline = rs.pipeline()
        self.config   = rs.config()

        # 스트림 설정
        self.config.enable_stream(rs.stream.depth, self.width, self.height, rs.format.z16, self.fps)
        self.config.enable_stream(rs.stream.color, self.width, self.height, rs.format.bgr8, self.fps)

        self.is_init = False


    def init(self) -> bool:
        """ 카메라 초기화 """
        if not self.is_init:
            try:
                self.pipeline.start(self.config)
                self.is_init = True
                print(f"[Info ] [RealSense] 카메라 연결 성공")
                self.print_device_info()
                return True
            except RuntimeError as e:
                print(f"[Error] [RealSense] 카메라 연결 실패: {e}")
                return False

        # 이미 스트리밍 중인 경우
        return True
    
    def destroy(self):
        """ 카메라 종료 """
        if self.is_init:
            self.pipeline.stop()
            self.is_init = False
            print(f"[Info ] [RealSense] 카메라 종료")

    def get_frames(self) -> tuple[np.ndarray | None, np.ndarray | None]:
        """ 현재 프레임 리턴 """
        if not self.is_init:
            return None, None

        frames = self.pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()

        if not depth_frame or not color_frame:
            return None, None

        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        return color_image, depth_image  # numpy

    

    def print_device_info(self):
        """ 연결된 카메라의 장치 정보 출력 """
        ctx = rs.context()
        devices = ctx.query_devices()

        if len(devices) == 0:
            print("[Error] 연결된 RealSense 카메라를 찾을 수 없습니다.")
            return

        print("================================")
        for i, dev in enumerate(devices):
            name = dev.get_info(rs.camera_info.name)
            serial = dev.get_info(rs.camera_info.serial_number)
            firmware = dev.get_info(rs.camera_info.firmware_version)
            usb_type = dev.get_info(rs.camera_info.usb_type_descriptor)

            print(f"카메라 {i+1} : {name}")
            print(f"  - 시리얼 번호 : {serial}")
            print(f"  - 펌웨어 버전 : {firmware}")
            print(f"  - USB 연결 타입: USB {usb_type}")
        print("================================\n")

    def print_supported_resolutions(self):
        """ 카메라 센서별 지원 해상도, FPS, 포맷 출력 """
        ctx = rs.context()
        devices = ctx.query_devices()

        if len(devices) == 0:
            return

        dev = devices[0] # 첫 번째 연결된 기기 기준
        print("\n=== [ 지원하는 해상도 및 프로필 ] ===")

        for sensor in dev.query_sensors():
            sensor_name = sensor.get_info(rs.camera_info.name)
            print(f"\n▶ {sensor_name} Sensor:")

            # 중복 출력을 방지하고 깔끔하게 보여주기 위해 set 자료구조 사용
            profiles = set()
            for profile in sensor.get_stream_profiles():
                if profile.is_video_stream_profile():
                    v_profile = profile.as_video_stream_profile()
                    stream_type = v_profile.stream_type().name
                    width = v_profile.width()
                    height = v_profile.height()
                    fps = v_profile.fps()
                    format_type = v_profile.format().name

                    profiles.add((stream_type, width, height, fps, format_type))

            # 해상도(넓이), FPS 순으로 내림차순 정렬하여 출력
            sorted_profiles = sorted(list(profiles), key=lambda x: (x[0], -x[1], -x[2], -x[3]))
            for p in sorted_profiles:
                print(f"  - 스트림: {p[0]:5} | 해상도: {p[1]:4} x {p[2]:4} | FPS: {p[3]:2} | 포맷: {p[4]}")

        print("\n===================================\n")

    


def main():
    camera = RealSenseCamera(width=848, height=480, fps=30)

    try:
        if not camera.init():
            return

        while True:
            # 프레임 데이터 읽기
            color_image, depth_image = camera.get_frames()

            if color_image is None or depth_image is None:
                continue

            # 뎁스 이미지 색상 매핑
            depth_colormap = cv2.applyColorMap(
                cv2.convertScaleAbs(depth_image, alpha=0.03),
                cv2.COLORMAP_JET
            )

            images = np.hstack((color_image, depth_colormap))


            # 카메라 데이터 시각화
            cv2.namedWindow('RealSense D405', cv2.WINDOW_AUTOSIZE)
            cv2.imshow('RealSense D405', images)

            key = cv2.waitKey(1)
            if key & 0xFF == ord('q') or key == 27:
                break

    except Exception as e:
        print(f"오류가 발생했습니다: {e}")

    finally:
        camera.destroy()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()