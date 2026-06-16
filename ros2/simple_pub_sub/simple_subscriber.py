import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class SimpleSubscriber(Node):
    def __init__(self):
        super().__init__('simple_subscriber')

        self.subscription = self.create_subscription(
            String,
            '/simple_topic',
            self.listener_callback,
            10)

    def listener_callback(self, msg):
        """ 토픽 수신 콜백"""
        self.get_logger().info('수신한 메시지: "%s"' % msg.data)


def main(args=None):
    # ROS 2 초기화
    rclpy.init(args=args)

    # 구독자 노드 생성
    minimal_subscriber = SimpleSubscriber()

    # 생성한 노드를 spin시켜 메시지 수신 대기 상태로 유지
    rclpy.spin(minimal_subscriber)

    # 노드 종료 (선택 사항: spin이 중단되면 명시적으로 소멸)
    minimal_subscriber.destroy_node()

    # 종료 시 ROS 2 정리
    rclpy.shutdown()


if __name__ == '__main__':
    main()