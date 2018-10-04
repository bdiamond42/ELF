/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "Pybind.h"

#include <stdint.h>

#include <spdlog/spdlog.h>
#include <iostream>

#include "elf/ai/tree_search/tree_search_options.h"
#include "elf/base/context.h"
#include "elf/base/game_context.h"
#include "elf/base/game_interface.h"
#include "elf/base/remote_context.h"
#include "elf/comm/comm.h"
#include "elf/logging/Pybind.h"
#include "elf/options/Pybind.h"
#include "elf/utils/pybind.h"
#include "elf/ai/tree_search/Pybind.h"

namespace {

inline std::string version() {
#ifdef GIT_COMMIT_HASH
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
  return TOSTRING(GIT_COMMIT_HASH) "_" TOSTRING(GIT_STAGED);
#else
  return "";
#endif
#undef STRINGIFY
#undef TOSTRING
}

void register_common_func(pybind11::module& m) {
  namespace py = pybind11;

  using comm::ReplyStatus;
  using elf::PointerInfo;
  using elf::AnyP;
  using elf::FuncMapBase;
  using elf::SharedMemData;
  using elf::SharedMemOptions;
  using elf::Size;
  using elf::Waiter;

  auto ref = py::return_value_policy::reference_internal;

  PB_INIT(PointerInfo)
    .def(py::init<>())
    PB_FIELD(p)
    PB_FIELD(type)
    PB_FIELD(stride)
  PB_END

  py::enum_<ReplyStatus>(m, "ReplyStatus")
      .value("SUCCESS", ReplyStatus::SUCCESS)
      .value("FAILED", ReplyStatus::FAILED)
      .value("UNKNOWN", ReplyStatus::UNKNOWN)
      .export_values();

  py::class_<Size>(m, "Size").def("vec", &Size::vec, ref);

  py::class_<SharedMemOptions>(m, "SharedMemOptions")
      .def(py::init<const std::string&, int>())
      .def("idx", &SharedMemOptions::getIdx)
      .def("batchsize", &SharedMemOptions::getBatchSize)
      .def("label", &SharedMemOptions::getLabel, ref)
      .def("setTimeout", &SharedMemOptions::setTimeout);

  py::class_<SharedMemData>(m, "SharedMemData")
      .def("__getitem__", &SharedMemData::get, ref)
      .def("effective_batchsize", &SharedMemData::getEffectiveBatchSize)
      .def("getSharedMemOptions", &SharedMemData::getSharedMemOptionsC)
      .def("info", &SharedMemData::info);

  py::class_<AnyP>(m, "AnyP")
      .def("info", &AnyP::info)
      .def("field", &AnyP::field, ref)
      .def("setData", &AnyP::setData, ref);

  py::class_<FuncMapBase>(m, "FuncMapBase")
      .def("batchsize", &FuncMapBase::getBatchSize)
      .def("name", &FuncMapBase::getName, ref)
      .def("sz", &FuncMapBase::getSize, ref)
      .def("type_name", &FuncMapBase::getTypeName)
      .def("type_size", &FuncMapBase::getSizeOfType);

  m.def("version", &version);
}

void register_game(pybind11::module& m) {
  namespace py = pybind11;

  using elf::GameContext;
  using elf::Options;
  using MsgOptions = elf::msg::Options;
  using elf::BatchReceiver;
  using elf::BatchSender;
  using elf::EnvSender;
  using elf::GCInterface;

  auto ref = py::return_value_policy::reference_internal;

  PB_INIT(Options)
    .def(py::init<>())
    PB_FIELD(num_game_thread)
    PB_FIELD(batchsize)
    PB_FIELD(verbose)
    PB_FIELD(seed)
    PB_FIELD(job_id)
    PB_FIELD(time_signature)
  PB_END

  py::class_<MsgOptions>(m, "MsgOptions")
    .def(py::init<>());

  py::class_<GCInterface>(m, "GCInterface")
      .def("start", &GCInterface::start)
      .def("stop", &GCInterface::stop)
      .def(
          "wait",
          &GCInterface::wait,
          py::arg("timeout_usec") = 0,
          ref,
          py::call_guard<py::gil_scoped_release>())
      .def(
          "step",
          &GCInterface::step,
          py::arg("success") = comm::SUCCESS,
          py::call_guard<py::gil_scoped_release>())
      .def("allocateSharedMem", &GCInterface::allocateSharedMem, ref);

  py::class_<GameContext, GCInterface>(m, "GameContext")
      .def(py::init<const Options&>());

  py::class_<BatchSender, GameContext>(m, "BatchSender")
      .def(py::init<const Options&, const MsgOptions&>())
      .def("setRemoteLabels", &BatchSender::setRemoteLabels);

  py::class_<BatchReceiver, GCInterface>(m, "BatchReceiver")
      .def(py::init<const Options&, const MsgOptions&>());

  py::class_<EnvSender>(m, "EnvSender")
      .def(py::init<const Options&, const MsgOptions&>())
      .def("sendAndWaitReply", &EnvSender::sendAndWaitReply, ref);
}

} // namespace

namespace elf {

void registerPy(pybind11::module& m) {
  register_common_func(m);

  auto m_logging = m.def_submodule("_logging");
  elf::logging::registerPy(m_logging);

  auto m_options = m.def_submodule("_options");
  elf::options::registerPy(m_options);

  auto m_mcts = m.def_submodule("_mcts");
  elf::ai::tree_search::registerPy(m_mcts);

  register_game(m);
}

} // namespace elf
