/************************** BEGIN LibavReader.h **************************/
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2020 GRAME, Centre National de Creation Musicale
 Copyright (C) 2020 Jean-Michaël Celerier, ossia.io
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __LibavReader__
#define __LibavReader__

#if __has_include(<libavcodec/avcodec.h>)
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
}
#else
#error "libav headers not found"
#endif

#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>

#include "faust/gui/Soundfile.h"

struct LibavReader : public SoundfileReader {
  using audio_array = std::vector<std::vector<float>>;
  static audio_array data;
    struct AVCodecContext_Free {
      void operator()(AVCodecContext* ctx) const noexcept
      {
        avcodec_free_context(&ctx);
      }
    };
    struct AVFormatContext_Free {
      void operator()(AVFormatContext* ctx) const noexcept
      {
        avformat_close_input(&ctx);
      }
    };
    struct AVFrame_Free {
      void operator()(AVFrame* frame) const noexcept
      {
        av_frame_free(&frame);
      }
    };
    struct SwrContext_Free {
      void operator()(AVFrame* frame) const noexcept
      {
        av_frame_free(&frame);
      }
    };
    using AVFormatContext_ptr = std::unique_ptr<AVFormatContext, AVFormatContext_Free>;
    using AVCodecContext_ptr = std::unique_ptr<AVCodecContext, AVCodecContext_Free>;
    using AVFrame_ptr = std::unique_ptr<AVFrame, AVFrame_Free>;
    using SwrContext_ptr = std::unique_ptr<SwrContext, SwrContext_Free>;

    LibavReader() {
      av_register_all();
      avcodec_register_all();
    }

    // Check file
    bool checkFile(const std::string& path_name)
    {
        AVFormatContext* fmtContext = nullptr;
        auto ret = avformat_open_input(&fmtContext, path_name.c_str(), nullptr, nullptr);
        if (ret != 0)
        {
          char err[100]{0};
          av_make_error_string(err, 100, ret);
          std::cerr << "LibavReader: Couldn't open file: '"
                    << path_name << "': "
                    << err << "\n";
          return false;
        }
        avformat_close_input(&fmtContext);
        return true;
    }

    bool checkFile(unsigned char* buffer, size_t length)
    {
        return false;
    }

    // Open the file and returns its length and channels
    void getParamsFile(const std::string& path_name, int& channels, int& length)
    {
        channels = 0;
        length = 0;

        AVFormatContext* fmtContext = nullptr;
        auto ret = avformat_open_input(&fmtContext, path_name.c_str(), nullptr, nullptr);
        AVFormatContext_ptr fmtContext_p{fmtContext};

        if (avformat_find_stream_info(fmtContext, nullptr) < 0)
          return;

        for (std::size_t i = 0; i < fmtContext->nb_streams; i++)
        {
          const auto stream = fmtContext->streams[i];
          if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            channels = stream->codecpar->channels;
            if (channels <= 0)
              continue;

            auto rate = stream->codecpar->sample_rate;
            length = std::ceil(rate * fmtContext->duration / double(AV_TIME_BASE));
            break;
          }
        }
    }

    void getParamsFile(unsigned char* buffer, size_t size, int& channels, int& length)
    {
        return;
    }

    void readFile(Soundfile* soundfile, const std::string& path_name, int part, int& offset, int max_chan)
    {
        AVFormatContext* fmtContext = nullptr;
        auto ret = avformat_open_input(&fmtContext, path_name.c_str(), nullptr, nullptr);
        AVFormatContext_ptr fmt_ctx{fmtContext};

        if (avformat_find_stream_info(fmtContext, nullptr) < 0)
          return;

        int channels = 0;
        int length = 0;
        int rate = 0;
        AVSampleFormat format = {};

        AVStream* audioStream = nullptr;


        // Find the correct stream
        for (std::size_t i = 0; i < fmtContext->nb_streams; i++)
        {
          const auto stream = fmtContext->streams[i];
          if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            channels = stream->codecpar->channels;
            if (channels <= 0)
              continue;

            rate = stream->codecpar->sample_rate;
            length = std::ceil(rate * fmtContext->duration / double(AV_TIME_BASE));
            format = (AVSampleFormat)stream->codecpar->format;
            audioStream = stream;
            break;
          }
        }
        if (!audioStream)
          return;

        // libav setup...
        auto codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
        if (!codec) {
          std::cerr << "LibavReader: Couldn't find codec\n";
          return;
        }

        AVCodecContext_ptr codec_ctx{avcodec_alloc_context3(codec)};
        if (!codec_ctx) {
          std::cerr << "LibavReader: Couldn't allocate codec context\n";
          return;
        }

        ret = avcodec_parameters_to_context(codec_ctx.get(), audioStream->codecpar);
        if (ret != 0) {
          std::cerr << "LibavReader: Couldn't copy codec data\n";
          return;
        }

        ret = avcodec_open2(codec_ctx.get(), codec, nullptr);
        if (ret != 0) {
          std::cerr << "LibavReader: Couldn't open codec\n";
          return;
        }

        // Create resampler / converter
        constexpr auto out_format = std::is_same_v<FAUSTFLOAT, double>
          ? AV_SAMPLE_FMT_DBLP
          : AV_SAMPLE_FMT_FLTP
        ;

        auto swr = SwrContext_ptr{swr_alloc_set_opts(
            nullptr,
            AV_CH_LAYOUT_MONO, out_format, fDriverSR,
            audioStream->codecpar->channel_layout, format, rate,
            0, nullptr)};
        swr_init(swr.get());

        // Main decoding loop
        {
          AVPacket packet;
          AVFrame_ptr frame{av_frame_alloc()};

          ret = av_read_frame(fmt_ctx.get(), &packet);

          debug(ret, "av_read_frame");
          int update = 0;
          while (ret >= 0)
          {
            ret = avcodec_send_packet(codec_ctx.get(), &packet);
            debug(ret, "avcodec_send_packet");
            if (ret == 0)
            {
              ret = avcodec_receive_frame(codec_ctx.get(), frame.get());
              debug(ret, "avcodec_receive_frame");
              if (ret == 0)
              {
                while (ret == 0)
                {
                  decode(data, *frame, rate, *swr);
                  ret = avcodec_receive_frame(codec_ctx.get(), frame.get());
                }

                ret = av_read_frame(fmt_ctx.get(), &packet);
                debug(ret, "av_read_frame");
                continue;
              }
              else if (ret == AVERROR(EAGAIN))
              {
                ret = av_read_frame(fmt_ctx.get(), &packet);
                debug(ret, "av_read_frame");
                continue;
              }
              else if (ret == AVERROR_EOF)
              {
                decode(data, *frame, rate, *swr);
                break;
              }
              else
              {
                break;
              }
            }
            else if (ret == AVERROR(EAGAIN))
            {
              ret = avcodec_receive_frame(codec_ctx.get(), frame.get());
              debug(ret, "avcodec_receive_frame EAGAIN");
            }
            else
            {
              break;
            }
          }

          // Flush
          ret = avcodec_send_packet(codec_ctx.get(), nullptr);

          //decodeRemaining(dec, data, *frame);
        }
    }

    void readFile(Soundfile* soundfile, unsigned char* buffer, size_t length, int part, int& offset, int max_chan)
    {
    }

private:
    void decode(audio_array& data, AVFrame& frame, double rate, SwrContext& swr)
    {
      const std::size_t channels = data.size();
      const std::size_t max_samples = data[0].size();

      auto new_len = av_rescale_rnd(frame.nb_samples, fDriverSR, rate, AV_ROUND_UP);

      auto samples = frame.nb_samples;

      FAUSTFLOAT**v = (FAUSTFLOAT**)alloca(sizeof(FAUSTFLOAT*) * channels);
      for(int i = 0; i < channels; i++) {
        v[i] = (FAUSTFLOAT*)alloca(sizeof(FAUSTFLOAT) * max_samples);
      }

      swr_convert(&swr,
            (uint8_t**)v, new_len,
            (const uint8_t**)frame.data, frame.nb_samples);

    }

    void debug(int ret, const char* ctx)
    {
      if (ret < 0 && ret != AVERROR_EOF)
      {
        char err[1024]{0};
        av_make_error_string(err, 1024, ret);
        std::cerr << "LibavReader: " << ctx << ": error " << ret << err << "\n";
      }
    }
};

#endif
/**************************  END  LibavReader.h **************************/
