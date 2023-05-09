#include "asio.hpp"
#include "message.hpp"
#include "channel.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

using asio::ip::tcp;

namespace img {
constexpr std::size_t X = 640;
constexpr std::size_t Y = 480;
constexpr std::size_t C = 3;
constexpr std::size_t SIZE = 640 * 480 * 3;
} // namespace img

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " HOST PORT\n";
    return 1;
  }

  std::string host(argv[1]);
  std::string port(argv[2]);

  Channel<std::string> ch_error;
  std::thread error_thread([&ch_error] {
    std::string err;
    while (ch_error.recv(err)) {
      std::cerr << err << '\n';
    }
  });

  Channel<cv::Mat> ch_video;
  std::thread net_thread([&ch_video, &ch_error, &host, &port] {
    try {
      asio::io_context io_context;

      tcp::socket s(io_context);
      tcp::resolver resolver(io_context);
      asio::connect(s, resolver.resolve(host, port));

      unsigned char len_buf[4];
      std::vector<unsigned char> buf;
      std::uint32_t len;
      asio::error_code ec;
      for (;;) {
        asio::read(s, asio::buffer(len_buf, sizeof(len)), ec);
        if (ec == asio::error::eof) {
          break;
        } else if (ec) {
          ch_error.send("net_thread: " + ec.message());
          break;
        }


	len = decode<std::uint32_t>(len_buf);
	buf.resize(len);

        asio::read(s, asio::buffer(buf), ec);
        if (ec == asio::error::eof) {
          break;
        } else if (ec) {
          ch_error.send("net_thread: " + ec.message());
          break;
        }

	cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);

        if (!ch_video.send(frame)) {
          ch_error.send("net_thread: ch_video is closed");
	  return;
        }
      }
    } catch (std::exception &e) {
      ch_error.send(std::string("net_thread: ") + e.what());
      ch_video.close();
    }
  });

  std::thread video_thread([&ch_video, &ch_error] {
    cv::Mat frame;
    int key = 0;
    while (key != 'q') {
      if (!ch_video.recv(frame)) {
        ch_error.send("video_thread: ch_video is closed");
        return;
      }

      cv::imshow("client", frame);
      key = cv::waitKey(1);
    }

    ch_video.close();
  });

  net_thread.join();
  video_thread.join();
  ch_error.close();
  error_thread.join();
}
