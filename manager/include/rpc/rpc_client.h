#pragma once

#include <string>

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>

#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"
#include "query_api.grpc.pb.h"

namespace monitor {

class RpcClient {
 public:
  explicit RpcClient(const std::string& host_address = "localhost:50051");
  ~RpcClient();

  bool GetMonitorInfo(monitor::proto::MonitorInfo* monitor_info);

  bool QueryPerformance(
      const monitor::proto::QueryPerformanceRequest& request,
      monitor::proto::QueryPerformanceResponse* response);

  const std::string& GetHostAddress() const { return host_address_; }

 private:
  std::unique_ptr<monitor::proto::GrpcManager::Stub> stub_ptr_;
  std::unique_ptr<monitor::proto::QueryService::Stub> query_stub_;
  std::string host_address_;
};

}  // namespace monitor