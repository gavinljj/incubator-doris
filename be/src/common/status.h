// Modifications copyright (C) 2017, Baidu.com, Inc.
// Copyright 2017 The Apache Software Foundation

// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef BDG_PALO_BE_SRC_COMMON_COMMON_STATUS_H
#define BDG_PALO_BE_SRC_COMMON_COMMON_STATUS_H

#include <string>
#include <vector>

#include "common/logging.h"
#include "common/compiler_util.h"
#include "gen_cpp/Status_types.h"  // for TStatus

namespace palo {

// Status is used as a function return type to indicate success, failure or cancellation
// of the function. In case of successful completion, it only occupies sizeof(void*)
// statically allocated memory. In the error case, it records a stack of error messages.
//
// example:
// Status fnB(int x) {
//   Status status = fnA(x);
//   if (!status.ok()) {
//     status.AddErrorMsg("fnA(x) went wrong");
//     return status;
//   }
// }
//
// TODO: macros:
// RETURN_IF_ERROR(status) << "msg"
// MAKE_ERROR() << "msg"

class Status {
public:
    Status(): _error_detail(NULL) {}

    static const Status OK;
    static const Status CANCELLED;
    static const Status MEM_LIMIT_EXCEEDED;
    static const Status THRIFT_RPC_ERROR;

    // copy c'tor makes copy of error detail so Status can be returned by value
    Status(const Status& status) : _error_detail(
            status._error_detail != NULL
            ? new ErrorDetail(*status._error_detail)
            : NULL) {
    }

    // c'tor for error case - is this useful for anything other than CANCELLED?
    Status(TStatusCode::type code) : _error_detail(new ErrorDetail(code)) {
    }

    // c'tor for error case
    Status(TStatusCode::type code, const std::string& error_msg, bool quiet) :
            _error_detail(new ErrorDetail(code, error_msg)) {
        if (!quiet) {
            VLOG(2) << error_msg;
        }
    }

    Status(TStatusCode::type code, const std::string& error_msg);

    // c'tor for internal error
    Status(const std::string& error_msg);

    Status(const std::string& error_msg, bool quiet);

    ~Status() {
        if (_error_detail != NULL) {
            delete _error_detail;
        }
    }

    // same as copy c'tor
    Status& operator=(const Status& status) {
        delete _error_detail;

        if (LIKELY(status._error_detail == NULL)) {
            _error_detail = NULL;
        } else {
            _error_detail = new ErrorDetail(*status._error_detail);
        }

        return *this;
    }

    // "Copy" c'tor from TStatus.
    Status(const TStatus& status);

    // same as previous c'tor
    Status& operator=(const TStatus& status);

    // assign from stringstream
    Status& operator=(const std::stringstream& stream);

    bool ok() const {
        return _error_detail == NULL;
    }

    bool is_cancelled() const {
        return _error_detail != NULL
               && _error_detail->error_code == TStatusCode::CANCELLED;
    }

    bool is_mem_limit_exceeded() const {
        return _error_detail != NULL
               && _error_detail->error_code == TStatusCode::MEM_LIMIT_EXCEEDED;
    }

    bool is_thrift_rpc_error() const {
        return _error_detail != NULL
               && _error_detail->error_code == TStatusCode::MEM_LIMIT_EXCEEDED;
    }

    // Add an error message and set the code if no code has been set yet.
    // If a code has already been set, 'code' is ignored.
    void add_error_msg(TStatusCode::type code, const std::string& msg);

    // Add an error message and set the code to INTERNAL_ERROR if no code has been
    // set yet. If a code has already been set, it is left unchanged.
    void add_error_msg(const std::string& msg);

    // Does nothing if status.ok().
    // Otherwise: if 'this' is an error status, adds the error msg from 'status;
    // otherwise assigns 'status'.
    void add_error(const Status& status);

    // Return all accumulated error msgs.
    void get_error_msgs(std::vector<std::string>* msgs) const;

    // Convert into TStatus. Call this if 'status_container' contains an optional
    // TStatus field named 'status'. This also sets __isset.status.
    template <typename T> void set_t_status(T* status_container) const {
        to_thrift(&status_container->status);
        status_container->__isset.status = true;
    }

    // Convert into TStatus.
    void to_thrift(TStatus* status) const;

    // Return all accumulated error msgs in a single string.
    void get_error_msg(std::string* msg) const;

    std::string get_error_msg() const;

    TStatusCode::type code() const {
        return _error_detail == NULL ? TStatusCode::OK : _error_detail->error_code;
    }

private:
    struct ErrorDetail {
        TStatusCode::type error_code;  // anything other than OK
        std::vector<std::string> error_msgs;

        ErrorDetail(const TStatus& status);
        ErrorDetail(TStatusCode::type code)
            : error_code(code) {}
        ErrorDetail(TStatusCode::type code, const std::string& msg)
            : error_code(code), error_msgs(1, msg) {}
    };

    ErrorDetail* _error_detail;
};

// some generally useful macros
#define RETURN_IF_ERROR(stmt) \
    do { \
        Status _status_ = (stmt); \
        if (UNLIKELY(!_status_.ok())) { \
            return _status_; \
        } \
    } while (false)

#define EXIT_IF_ERROR(stmt) \
    do { \
        Status _status_ = (stmt); \
        if (UNLIKELY(!_status_.ok())) { \
            string msg; \
            _status_.get_error_msg(&msg); \
            LOG(ERROR) << msg;            \
            exit(1); \
        } \
    } while (false)

}

#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#endif