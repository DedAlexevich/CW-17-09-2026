#include <iostream>
#include <pthread.h>
#include <random>
#include <string.h>
#include <vector>
#include <stdexcept>

namespace kuznetsov{
  struct arg_t {
    double* r;
    size_t* tests;
    size_t seed;
    size_t result;
  };

  struct Thread_Guard {
    std::vector< pthread_t >& ths;
    size_t& created;
    size_t& start;
    ~Thread_Guard() {
      for (size_t i = start; i < created; ++i) {
        pthread_join(ths[i], nullptr);
      }
    }
  };
  
  double area(double r, size_t threads, size_t tests);
  size_t calc(double r, size_t tests, size_t seed);
  bool isInside(double x, double y, double r);
  void* proxyCalc(void*);
}

int main(int argc, char** argv)
{
  if (argc < 4) {
    std::cerr << "Not enought args\n";
    return 1;
  }
  size_t threads, tests;
  double r;
  threads = std::stoull(argv[1]);
  tests = std::stoull(argv[2]);
  r = std::stod(argv[3]);
  
  if (threads == 0 || tests == 0 || r <= 0) {
    std::cerr << "threads, tests and radius must be >0\n";
    return 1;
  }
  
  double ar;
  try {
    ar = kuznetsov::area(r, threads, tests);
  } catch(const std::runtime_error& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  std::cout << ar << '\n';
}

bool kuznetsov::isInside(double x, double y, double r)
{
  return (r - x) * (r - x) + (r - y) * (r - y) <= r * r;
}

size_t kuznetsov::calc(double r, size_t tests, size_t seed)
{
  std::default_random_engine eng(seed);
  std::uniform_real_distribution<double> dist(0, 2 * r);
  size_t res = 0;
  for (size_t i = 0; i < tests; ++i) {
    res += isInside(dist(eng), dist(eng), r);
  }
  return res;
}

double kuznetsov::area(double r, size_t threads, size_t tests)
{
  size_t sumTests = tests * threads;
  size_t insided = 0;
  
  std::vector< pthread_t > thrds(threads);
  std::vector< arg_t > args(threads, {&r, &tests, 0, 0});
  
  size_t created = 0, start = 0;
  Thread_Guard tg {thrds, created, start};

  for (; created < threads; ++created) {
    args[created].seed = created;
    int err = pthread_create(&thrds[created], nullptr, proxyCalc, &args[created]);
    if (err != 0) {
      throw std::runtime_error(strerror(err));
    }
  }
  
  for(size_t j = 0; j < created; ++j) {
    int err = pthread_join(thrds[j], nullptr);
    if (err) {
      throw std::runtime_error(strerror(err));
    }
    ++start;
    insided += args[j].result;
  }

  return (2 * r) * (2 * r) * insided / static_cast< double >(sumTests);
}

void* kuznetsov::proxyCalc(void* arg)
{
  namespace kuz = kuznetsov;
  kuz::arg_t* arguments = static_cast< kuz::arg_t* >(arg);
  arguments->result = kuz::calc(*(arguments->r), *(arguments->tests), arguments->seed);
  return nullptr;
}

