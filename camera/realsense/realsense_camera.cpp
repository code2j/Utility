#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <algorithm>
#include <iomanip>

class RealSenseCamera {
private:
    int width;
    int height;
    int fps;

    rs2::pipeline pipeline;
    rs2::config config;
    bool is_init;

public:
    RealSenseCamera(int w = 848, int h = 480, int f = 30)
        : width(w), height(h), fps(f), is_init(false) {

        // 스트림 설정
        config.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps);
        config.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_BGR8, fps);
    }

    // 카메라 초기화
    bool init() {
        if (!is_init) {
            try {
                pipeline.start(config);
                is_init = true;
                std::cout << "[Info ] [RealSense] 카메라 연결 성공\n";
                return true;
            } catch (const rs2::error & e) {
                std::cerr << "[Error] [RealSense] 카메라 연결 실패: " << e.what() << "\n";
                return false;
            } catch (const std::exception& e) {
                std::cerr << "[Error] " << e.what() << "\n";
                return false;
            }
        }
        return true; // 이미 스트리밍 중인 경우
    }

    // 카메라 종료
    void destroy() {
        if (is_init) {
            pipeline.stop();
            is_init = false;
            std::cout << "[Info ] [RealSense] 카메라 종료\n";
        }
    }

    // 현재 프레임 리턴 (참조로 OpenCV Mat 객체 전달)
    bool get_frames(cv::Mat& color_image, cv::Mat& depth_image) {
        if (!is_init) return false;

        rs2::frameset frames;
        if (!pipeline.poll_for_frames(&frames)) {
            // poll_for_frames 대신 wait_for_frames()를 써도 됩니다.
            frames = pipeline.wait_for_frames();
        }

        rs2::depth_frame depth_frame = frames.get_depth_frame();
        rs2::video_frame color_frame = frames.get_color_frame();

        if (!depth_frame || !color_frame) return false;

        // 포인터를 이용하여 OpenCV Mat으로 래핑
        depth_image = cv::Mat(cv::Size(width, height), CV_16UC1, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
        color_image = cv::Mat(cv::Size(width, height), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);

        return true;
    }

    // 연결된 카메라의 장치 정보 출력
    void print_device_info() {
        rs2::context ctx;
        auto devices = ctx.query_devices();

        if (devices.size() == 0) {
            std::cout << "⚠️ 연결된 RealSense 카메라를 찾을 수 없습니다.\n";
            return;
        }

        std::cout << "\n=== [ 카메라 장치 정보 ] ===\n";
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
        std::cout << "============================\n\n";
    }

    // 카메라 센서별 지원 해상도, FPS, 포맷 출력
    void print_supported_resolutions() {
        rs2::context ctx;
        auto devices = ctx.query_devices();

        if (devices.size() == 0) return;

        auto dev = devices[0]; // 첫 번째 연결된 기기 기준
        std::cout << "\n=== [ 지원하는 해상도 및 프로필 ] ===\n";

        for (auto&& sensor : dev.query_sensors()) {
            std::string sensor_name = sensor.get_info(RS2_CAMERA_INFO_NAME);
            std::cout << "\n▶ " << sensor_name << " Sensor:\n";

            // 중복 방지를 위한 set (Stream Type, Width, Height, FPS, Format)
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

            // 정렬을 위해 vector로 변환
            std::vector<ProfileData> sorted_profiles(profiles.begin(), profiles.end());
            std::sort(sorted_profiles.begin(), sorted_profiles.end(), [](const ProfileData& a, const ProfileData& b) {
                if (std::get<0>(a) != std::get<0>(b)) return std::get<0>(a) < std::get<0>(b); // 스트림 알파벳순
                if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) > std::get<1>(b); // Width 내림차순
                if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) > std::get<2>(b); // Height 내림차순
                return std::get<3>(a) > std::get<3>(b);                                       // FPS 내림차순
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

int main() {
    RealSenseCamera camera(848, 480, 30);

    camera.print_device_info();
    camera.print_supported_resolutions();

    try {
        if (!camera.init()) {
            return -1;
        }

        cv::Mat color_image, depth_image;

        cv::namedWindow("RealSense D405", cv::WINDOW_AUTOSIZE);

        while (true) {
            // 프레임 데이터 읽기
            if (!camera.get_frames(color_image, depth_image)) {
                continue;
            }

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
    } catch (const std::exception& e) {
        std::cerr << "오류가 발생했습니다: " << e.what() << "\n";
    }

    camera.destroy();
    cv::destroyAllWindows();

    return 0;
}