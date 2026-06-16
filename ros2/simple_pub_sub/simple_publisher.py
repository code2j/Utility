import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class SimplePublisher(Node):

    def __init__(self):
        super().__init__('simple_publisher')

        self.publisher_ = self.create_publisher(
            String,
            '/simple_topic',
            10
        )

        timer_period = 0.5 # 0.5초
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        """ 퍼블리셔 콜백 """
        msg = String()
        msg.data = 'Hello, world! %d' % self.i

        # 메시지 발행
        self.publisher_.publish(msg)

        # 로그 출력
        self.get_logger().info('발행하는 메시지: "%s"' % msg.data)
        self.i += 1


def main(args=None):
    rclpy.init(args=args)
    minimal_publisher = SimplePublisher()
    rclpy.spin(minimal_publisher)
    minimal_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()