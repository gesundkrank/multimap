
// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "multimap/jni/common.hpp"

#include <stdexcept>

namespace multimap {
namespace jni {

void propagateOrRethrow(JNIEnv* env, const std::exception& error) {
  if (env->ExceptionOccurred()) {
    // The Java code called before has thrown an exception.
    // Not calling env->ExceptionClear() will propagate it to the JVM.
  } else {
    throwJavaException(env, error.what());
  }
}

void throwJavaException(JNIEnv* env, const char* message) {
  const auto cls = env->FindClass("java/lang/Exception");
  mt::Check::notNull(cls, "FindClass() failed");
  env->ThrowNew(cls, message);
}

std::string toString(JNIEnv* env, jstring string) {
  const auto chars = env->GetStringUTFChars(string, nullptr);
  mt::Check::notNull(chars, "GetStringUTFChars() failed");
  std::string result = chars;
  env->ReleaseStringUTFChars(string, chars);
  return result;
}

Options toOptions(JNIEnv* env, jobject options) {
  MT_REQUIRE_NOT_NULL(options);
  const auto cls = env->GetObjectClass(options);
  mt::Check::notNull(cls, "GetObjectClass(options) failed");

  Options opts;
  const auto fid_numShards = env->GetFieldID(cls, "numShards", "I");
  mt::Check::notNull(fid_numShards, "GetFieldID(numShards) failed");
  opts.num_shards = env->GetIntField(options, fid_numShards);

  const auto fid_blockSize = env->GetFieldID(cls, "blockSize", "I");
  mt::Check::notNull(fid_blockSize, "GetFieldID(blockSize) failed");
  opts.block_size = env->GetIntField(options, fid_blockSize);

  const auto fid_createIfMissing = env->GetFieldID(cls, "createIfMissing", "Z");
  mt::Check::notNull(fid_createIfMissing, "GetFieldID(createIfMissing) failed");
  opts.create_if_missing = env->GetBooleanField(options, fid_createIfMissing);

  const auto fid_errorIfExists = env->GetFieldID(cls, "errorIfExists", "Z");
  mt::Check::notNull(fid_errorIfExists, "GetFieldID(errorIfExists) failed");
  opts.error_if_exists = env->GetBooleanField(options, fid_errorIfExists);

  const auto fid_readonly = env->GetFieldID(cls, "readonly", "Z");
  mt::Check::notNull(fid_readonly, "GetFieldID(readonly) failed");
  opts.readonly = env->GetBooleanField(options, fid_readonly);

  const auto fid_quiet = env->GetFieldID(cls, "quiet", "Z");
  mt::Check::notNull(fid_quiet, "GetFieldID(quiet) failed");
  opts.quiet = env->GetBooleanField(options, fid_quiet);

  const auto fid_lessThan =
      env->GetFieldID(cls, "lessThan", "Lio/multimap/Callables$LessThan;");
  mt::Check::notNull(fid_lessThan, "GetFieldID(lessThan) failed");
  const auto less_than = env->GetObjectField(options, fid_lessThan);
  if (less_than) {
    opts.compare = toCompare(env, less_than);
  }

  return opts;
}

Callables::Compare toCompare(JNIEnv* env, jobject less_than) {
  MT_REQUIRE_NOT_NULL(less_than);
  const auto cls = env->GetObjectClass(less_than);
  const auto mid = env->GetMethodID(
      cls, "call", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Z");
  mt::Check::notNull(mid, "GetMethodID() failed");
  return [=](const multimap::Bytes& lhs, const multimap::Bytes& rhs) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env->CallBooleanMethod(less_than, mid, newDirectByteBuffer(env, lhs),
                               newDirectByteBuffer(env, rhs));
    if (env->ExceptionOccurred()) {
      throw std::runtime_error("Exception in comparator passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    return result;
  };
}

Callables::Function toFunction(JNIEnv* env, jobject function) {
  MT_REQUIRE_NOT_NULL(function);
  const auto cls = env->GetObjectClass(function);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)[B");
  mt::Check::notNull(mid, "GetMethodID() failed");
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env->CallObjectMethod(function, mid, newDirectByteBuffer(env, bytes));
    if (env->ExceptionOccurred()) {
      throw std::runtime_error("Exception in function passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    // result is a jbyteArray that is copied into a std::string.
    return (result != nullptr) ? BytesRaiiHelper(env, result).get().toString()
                               : std::string();
  };
}

Callables::Predicate toPredicate(JNIEnv* env, jobject predicate) {
  MT_REQUIRE_NOT_NULL(predicate);
  const auto cls = env->GetObjectClass(predicate);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)Z");
  mt::Check::notNull(mid, "GetMethodID() failed");
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env->CallBooleanMethod(predicate, mid, newDirectByteBuffer(env, bytes));
    if (env->ExceptionOccurred()) {
      throw std::runtime_error("Exception in predicate passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    return result;
  };
}

Callables::Procedure toProcedure(JNIEnv* env, jobject procedure) {
  MT_REQUIRE_NOT_NULL(procedure);
  const auto cls = env->GetObjectClass(procedure);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)V");
  mt::Check::notNull(mid, "GetMethodID() failed");
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    env->CallVoidMethod(procedure, mid, newDirectByteBuffer(env, bytes));
    if (env->ExceptionOccurred()) {
      throw std::runtime_error("Exception in procedure passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
  };
}

} // namespace jni
} // namespace multimap