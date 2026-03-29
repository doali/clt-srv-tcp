#pragma once
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

namespace asio = boost::asio;

namespace netlib
{
// Boost.Asio => boost::system::error_code (pas std::error_code).
// [1](blob:https://outlook.office.com/0090b658-72fe-4067-90f0-9cf3f1159eb8)[2](https://edfonline-my.sharepoint.com/personal/marc-externe_hermitte_edf_fr/_layouts/15/Doc.aspx?sourcedoc=%7B767FB310-3159-44D8-8F22-1D2A8E6B6A52%7D&file=Exigences%20techniques%20(PTH3.PROD.00257).doc&action=default&mobileredirect=true&DefaultItemOpen=1)
using ErrorCode = boost::system::error_code;
} // namespace netlib
