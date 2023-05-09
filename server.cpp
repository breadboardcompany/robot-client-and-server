#include "asio.hpp"
#include "message.hpp"
#include "channel.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <thread>
#include <vector>

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
  std::thread video_thread([&ch_video, &ch_error] {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
      ch_error.send("video_thread: could not open camera");
      ch_video.close();
      return;
    }

    cv::Mat frame;
    if (!cap.read(frame)) {
      ch_error.send("video_thread: could not read");
      ch_video.close();
      return;
    } else if (frame.type() != CV_8UC3) {
      ch_error.send("video_thread: data type is not CV_8UC3");
      ch_video.close();
      return;
    }

    for (;;) {
      if (!cap.read(frame)) {
        ch_error.send("video_thread: could not read");
        return;
      }

      cv::resize(frame, frame, cv::Size(img::X, img::Y));
      if (!ch_video.send(std::move(frame))) {
        ch_error.send("video_thread: ch_video is closed");
        return;
      }
    }
  });

  std::thread net_thread([&ch_video, &ch_error, &host, &port] {
    try {
      asio::io_context io_context;

      tcp::resolver resolver(io_context);
      tcp::endpoint endpoint = *resolver.resolve(host, port).begin();

      tcp::acceptor a(io_context, endpoint);

      cv::Mat frame;
      unsigned char len_buf[4];
      std::vector<unsigned char> buf;
      std::uint32_t len;
      asio::error_code ec;
      for (;;) {
        auto s = a.accept();

	asio::socket_base::send_buffer_size option;
	s.get_option(option);
	std::cout << option.value() << '\n';

        for (;;) {
          if (!ch_video.recv(frame)) {
            ch_error.send("net_thread: ch_video is closed");
	    return;
          }

	  cv::imencode(".jpg", frame, buf);

	  len = buf.size();

	  encode<std::uint32_t>(len_buf, len);
          asio::write(s, asio::buffer(len_buf, sizeof(len)), ec);
          if (ec == asio::error::eof) {
            break;
          } else if (ec) {
            ch_error.send("net_thread: " + ec.message());
            break;
          }

          asio::write(s, asio::buffer(buf, len), ec);
          if (ec == asio::error::eof) {
            break;
          } else if (ec) {
            ch_error.send("net_thread: " + ec.message());
            break;
          }
        }
      }
    } catch (std::exception &e) {
      ch_error.send(std::string("net_thread: ") + e.what());
      ch_video.close();
    }
  });

  video_thread.join();
  net_thread.join();
  ch_error.close();
  error_thread.join();
}
