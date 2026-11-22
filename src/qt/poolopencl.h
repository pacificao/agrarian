// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_POOLOPENCL_H
#define AGRARIAN_QT_POOLOPENCL_H

#include "poolwork.h"

#include <QByteArray>
#include <QLibrary>
#include <QString>
#if defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#endif

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <stdexcept>

namespace poolopencl {

// Runtime-loaded OpenCL: an absent GPU driver must not break CPU mode.
class Scanner {
public:
    explicit Scanner(const QByteArray& source, bool softwareTestOnly)
    {
#ifdef Q_OS_WIN
        wchar_t systemDir[1024]{};
        const auto chars = GetSystemDirectoryW(systemDir, 1024);
        if (chars == 0 || chars >= 1024) throw std::runtime_error("Cannot locate Windows System32");
        library.setFileName(QString::fromWCharArray(systemDir, chars) + QStringLiteral("/OpenCL.dll"));
#else
        library.setFileName(QStringLiteral("libOpenCL.so.1"));
#endif
        if (!library.load()) throw std::runtime_error("OpenCL driver is unavailable");
#define AGR_BIND(symbol) p_##symbol = reinterpret_cast<decltype(p_##symbol)>(library.resolve(#symbol)); \
        if (!p_##symbol) throw std::runtime_error("Incomplete OpenCL 1.2 runtime");
        AGR_BIND(clGetPlatformIDs) AGR_BIND(clGetDeviceIDs) AGR_BIND(clCreateContext)
        AGR_BIND(clCreateCommandQueue) AGR_BIND(clCreateProgramWithSource) AGR_BIND(clBuildProgram)
        AGR_BIND(clCreateKernel) AGR_BIND(clCreateBuffer) AGR_BIND(clSetKernelArg)
        AGR_BIND(clEnqueueWriteBuffer) AGR_BIND(clEnqueueNDRangeKernel) AGR_BIND(clEnqueueReadBuffer)
        AGR_BIND(clReleaseMemObject) AGR_BIND(clReleaseKernel) AGR_BIND(clReleaseProgram)
        AGR_BIND(clReleaseCommandQueue) AGR_BIND(clReleaseContext)
#undef AGR_BIND
        std::array<cl_platform_id, 32> platforms{};
        cl_uint platformCount = 0;
        if (p_clGetPlatformIDs(platforms.size(), platforms.data(), &platformCount) != CL_SUCCESS)
            throw std::runtime_error("No usable OpenCL platform");
        const cl_device_type type = softwareTestOnly ? CL_DEVICE_TYPE_CPU : CL_DEVICE_TYPE_GPU;
        for (size_t i = 0; i < std::min<size_t>(platformCount, platforms.size()); ++i) {
            std::array<cl_device_id, 32> devices{};
            cl_uint deviceCount = 0;
            if (p_clGetDeviceIDs(platforms[i], type, devices.size(), devices.data(), &deviceCount) != CL_SUCCESS)
                continue;
            for (size_t j = 0; j < std::min<size_t>(deviceCount, devices.size()); ++j) {
                try {
                    Init(devices[j], source);
                    SelfTest();
                    return;
                } catch (const std::runtime_error&) {
                    Release();
                }
            }
        }
        throw std::runtime_error("No OpenCL device passed the Quark startup test");
    }

    Scanner(const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;
    ~Scanner() { Release(); }

    poolwork::BatchResult Scan(const poolwork::Header& header, const poolwork::Target& target,
                               uint64_t first)
    {
        if (first >= poolwork::kNonceEnd) return {poolwork::kNonceEnd, 0, false, 0};
        cl_ulong high = 0;
        for (size_t i = 24; i < 32; ++i) high |= cl_ulong{target[i]} << (8 * (i - 24));
        const auto flags = Flags(header, static_cast<cl_uint>(first), high);
        const auto count = static_cast<uint32_t>(std::min<uint64_t>(kBatch, poolwork::kNonceEnd - first));
        for (uint32_t i = 0; i < count; ++i) {
            const auto nonce = static_cast<uint32_t>(first + i);
            if (flags[i] && poolwork::Verify(header, target, nonce))
                return {first + i + 1, kBatch, true, nonce};
        }
        return {first + count, kBatch, false, 0};
    }

private:
    static constexpr cl_uint kBatch = 256;
    QLibrary library;
#define AGR_POINTER(symbol) decltype(&symbol) p_##symbol{nullptr};
    AGR_POINTER(clGetPlatformIDs) AGR_POINTER(clGetDeviceIDs) AGR_POINTER(clCreateContext)
    AGR_POINTER(clCreateCommandQueue) AGR_POINTER(clCreateProgramWithSource) AGR_POINTER(clBuildProgram)
    AGR_POINTER(clCreateKernel) AGR_POINTER(clCreateBuffer) AGR_POINTER(clSetKernelArg)
    AGR_POINTER(clEnqueueWriteBuffer) AGR_POINTER(clEnqueueNDRangeKernel) AGR_POINTER(clEnqueueReadBuffer)
    AGR_POINTER(clReleaseMemObject) AGR_POINTER(clReleaseKernel) AGR_POINTER(clReleaseProgram)
    AGR_POINTER(clReleaseCommandQueue) AGR_POINTER(clReleaseContext)
#undef AGR_POINTER
    cl_context context{nullptr};
    cl_command_queue queue{nullptr};
    cl_program program{nullptr};
    cl_kernel kernel{nullptr};
    cl_mem input{nullptr}, output{nullptr};

    static void Check(cl_int status)
    {
        if (status != CL_SUCCESS) throw std::runtime_error("OpenCL Quark operation failed");
    }

    void Init(cl_device_id device, const QByteArray& source)
    {
        cl_int error = CL_SUCCESS;
        context = p_clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
        Check(error);
        queue = p_clCreateCommandQueue(context, device, 0, &error);
        Check(error);
        const auto* text = source.constData();
        const size_t length = static_cast<size_t>(source.size());
        program = p_clCreateProgramWithSource(context, 1, &text, &length, &error);
        Check(error);
        Check(p_clBuildProgram(program, 1, &device, "-cl-std=CL1.2 -DWORKSIZE=8", nullptr, nullptr));
        kernel = p_clCreateKernel(program, "search", &error);
        Check(error);
        input = p_clCreateBuffer(context, CL_MEM_READ_ONLY, poolwork::Header{}.size(), nullptr, &error);
        Check(error);
        output = p_clCreateBuffer(context, CL_MEM_WRITE_ONLY, kBatch, nullptr, &error);
        Check(error);
        Check(p_clSetKernelArg(kernel, 0, sizeof(input), &input));
        Check(p_clSetKernelArg(kernel, 1, sizeof(output), &output));
    }

    std::array<unsigned char, kBatch> Flags(const poolwork::Header& header, cl_uint first, cl_ulong high)
    {
        constexpr size_t global = kBatch, local = 8;
        Check(p_clEnqueueWriteBuffer(queue, input, CL_TRUE, 0, header.size(), header.data(), 0, nullptr, nullptr));
        Check(p_clSetKernelArg(kernel, 2, sizeof(first), &first));
        Check(p_clSetKernelArg(kernel, 3, sizeof(high), &high));
        Check(p_clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr));
        std::array<unsigned char, kBatch> flags{};
        Check(p_clEnqueueReadBuffer(queue, output, CL_TRUE, 0, flags.size(), flags.data(), 0, nullptr, nullptr));
        return flags;
    }

    void SelfTest()
    {
        poolwork::Header header{};
        const auto exact = poolwork::Digest(header, 0);
        cl_ulong threshold = 0;
        for (size_t i = 24; i < 32; ++i) threshold |= cl_ulong{exact.begin()[i]} << (8 * (i - 24));
        auto flags = Flags(header, 0, threshold);
        for (cl_uint nonce = 0; nonce < kBatch; ++nonce) {
            const auto hash = poolwork::Digest(header, nonce);
            cl_ulong high = 0;
            for (size_t i = 24; i < 32; ++i) high |= cl_ulong{hash.begin()[i]} << (8 * (i - 24));
            if (flags[nonce] != (high <= threshold)) throw std::runtime_error("OpenCL Quark result disagrees with Core");
        }
    }

    void Release()
    {
        if (output) p_clReleaseMemObject(output);
        if (input) p_clReleaseMemObject(input);
        if (kernel) p_clReleaseKernel(kernel);
        if (program) p_clReleaseProgram(program);
        if (queue) p_clReleaseCommandQueue(queue);
        if (context) p_clReleaseContext(context);
        output = input = nullptr;
        kernel = nullptr;
        program = nullptr;
        queue = nullptr;
        context = nullptr;
    }
};
} // namespace poolopencl
#endif
