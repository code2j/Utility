#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp> // main()에서만 사용
#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <algorithm>
#include <iomanip>

// 프레임 데이터를 외부로 전달하기 위한 구조체 정의
namespace RealSense
{
    struct FrameData {
        const void* color_data;
        const void* depth_data;
        int width;
        int height;
    };


    class RealSenseCamera {
    private:
        int width;
        int height;
        int fps;

        rs2::pipeline pipeline;
        rs2::config config;
        rs2::frameset last_frames; // 포인터 수명을 보장하기 위해 프레임셋 보관
        bool is_init;

    public:
        RealSenseCamera(int w = 848, int h = 480, int f = 30)
            : width(w), height(h), fps(f), is_init(false) {

            // 스트림 설정
            config.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps);
            config.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_BGR8, fps);
        }


        // 카메라 초기화
        bool init()
        {
            if (!is_init) {
                try {
                    pipeline.start(config);
                    is_init = true;
                    std::cout << "[Info ] [RealSense] 카메라 연결 성공\n";
                    print_device_info();
                    return true;
                } catch (const rs2::error& e) {
                    std::cerr << "[Error] [RealSense] 카메라 연결 실패: " << e.what() << "\n";
                    return false;
                }
            }
            return true;
        }


        // 카메라 종료
        void destroy()
        {
            if (is_init) {
                pipeline.stop();
                is_init = false;
                std::cout << "[Info ] [RealSense] 카메라 종료\n";
            }
        }


        // 현재 프레임 리턴 (구조체를 통해 원시 데이터 포인터 전달)
        bool get_frames(FrameData& out_data)
        {
            if (!is_init) return false;

            // 새로운 프레임 얻기 (last_frames에 저장하여 메모리 유지)
            if (!pipeline.poll_for_frames(&last_frames)) {
                last_frames = pipeline.wait_for_frames();
            }

            rs2::depth_frame depth_frame = last_frames.get_depth_frame();
            rs2::video_frame color_frame = last_frames.get_color_frame();

            if (!depth_frame || !color_frame) return false;

            // 구조체에 원시 포인터와 메타데이터 할당
            out_data.depth_data = depth_frame.get_data();
            out_data.color_data = color_frame.get_data();
            out_data.width = width;
            out_data.height = height;

            return true;
        }


        // 연결된 카메라의 장치 정보 출력
        void print_device_info()
        {
            rs2::context ctx;
            auto devices = ctx.query_devices();

            if (devices.size() == 0) {
                std::cout << "[Error] 연결된 RealSense 카메라를 찾을 수 없습니다.\n";
                return;
            }

            std::cout << "================================\n";
            for (size_t i = 0; i < devices.size(); ++i) {
                auto dev = devices[i];
                std::string name = dev.get_info(RS2_CAMERA_INFO_NAME);
                std::string serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                std::string firmware = dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION);
                std::string usb_type = dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);

                std::cout << "카메라 " << i + 1 << " : " << name << "\n";
                std::cout << "  - 시리얼 번호 : " << serial << "\n";
                std::cout << "  - 펌웨어 버전 : " << firmware << "\n";
                std::cout << "  - USB 연결 타입: USB " << usb_type << "\n";
            }
            std::cout << "================================\n\n";
        }


        // 카메라 센서별 지원 해상도, FPS, 포맷 출력
        void print_supported_resolutions()
        {
            rs2::context ctx;
            auto devices = ctx.query_devices();

            if (devices.size() == 0) return;

            auto dev = devices[0];
            std::cout << "\n=== [ 지원하는 해상도 및 프로필 ] ===\n";

            for (auto&& sensor : dev.query_sensors()) {
                std::string sensor_name = sensor.get_info(RS2_CAMERA_INFO_NAME);
                std::cout << "\n▶ " << sensor_name << " Sensor:\n";

                using ProfileData = std::tuple<std::string, int, int, int, std::string>;
                std::set<ProfileData> profiles;

                for (auto&& profile : sensor.get_stream_profiles()) {
                    if (auto v_profile = profile.as<rs2::video_stream_profile>()) {
                        std::string stream_type = rs2_stream_to_string(v_profile.stream_type());
                        int w = v_profile.width();
                        int h = v_profile.height();
                        int f = v_profile.fps();
                        std::string format_type = rs2_format_to_string(v_profile.format());

                        profiles.insert({stream_type, w, h, f, format_type});
                    }
                }

                std::vector<ProfileData> sorted_profiles(profiles.begin(), profiles.end());
                std::sort(sorted_profiles.begin(), sorted_profiles.end(), [](const ProfileData& a, const ProfileData& b) {
                    if (std::get<0>(a) != std::get<0>(b)) return std::get<0>(a) < std::get<0>(b);
                    if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) > std::get<1>(b);
                    if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) > std::get<2>(b);
                    return std::get<3>(a) > std::get<3>(b);
                });

                for (const auto& p : sorted_profiles) {
                    std::cout << "  - 스트림: " << std::left << std::setw(5) << std::get<0>(p)
                              << " | 해상도: " << std::right << std::setw(4) << std::get<1>(p)
                              << " x " << std::left << std::setw(4) << std::get<2>(p)
                              << " | FPS: " << std::right << std::setw(2) << std::get<3>(p)
                              << " | 포맷: " << std::get<4>(p) << "\n";
                }
            }
            std::cout << "\n===================================\n\n";
        }
    };
}


int main() {
    RealSense::RealSenseCamera camera(848, 480, 30);

    if (!camera.init()) {
        return -1;
    }

    cv::namedWindow("RealSense D405", cv::WINDOW_AUTOSIZE);

    // 데이터를 받아올 구조체 인스턴스 생성
    RealSense::FrameData frame_data;

    while (true) {
        // 카메라 클래스는 구조체에 원시 포인터만 채워줌 (OpenCV 의존성 제거)
        if (!camera.get_frames(frame_data)) {
            continue;
        }

        // 받아온 원시 포인터를 바탕으로 main 함수에서 OpenCV Mat 객체 래핑
        cv::Mat depth_image(cv::Size(frame_data.width, frame_data.height), CV_16UC1, (void*)frame_data.depth_data, cv::Mat::AUTO_STEP);
        cv::Mat color_image(cv::Size(frame_data.width, frame_data.height), CV_8UC3, (void*)frame_data.color_data, cv::Mat::AUTO_STEP);

        // 뎁스 이미지 색상 매핑 (alpha 0.03 스케일링)
        cv::Mat depth_scaled, depth_colormap;
        depth_image.convertTo(depth_scaled, CV_8UC1, 0.03);
        cv::applyColorMap(depth_scaled, depth_colormap, cv::COLORMAP_JET);

        // 이미지 가로로 붙이기 (hstack)
        cv::Mat images;
        cv::hconcat(std::vector<cv::Mat>{color_image, depth_colormap}, images);

        // 카메라 데이터 시각화
        cv::imshow("RealSense D405", images);

        int key = cv::waitKey(1);
        if ((key & 0xFF) == 'q' || key == 27) { // 'q' 또는 ESC
            break;
        }
    }

    camera.destroy();
    cv::destroyAllWindows();

    return 0;
}