#include "datetime.h"
#include "error.h"

#include <ctime>

namespace ledger {

std::time_t         now	      = std::time(NULL);
int                 now_year  = std::localtime(&now)->tm_year;

static std::time_t  base      = -1;
static int	    base_year = -1;

static const int    month_days[12] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const char * formats[] = {
  "%Y/%m/%d",
  "%m/%d",
  "%Y.%m.%d",
  "%m.%d",
  "%Y-%m-%d",
  "%m-%d",
  "%a",
  "%A",
  "%b",
  "%B",
  "%Y",
  NULL
};

std::time_t interval_t::first(const std::time_t moment)
{
  std::time_t quant = begin;

  if (moment && std::difftime(moment, quant) > 0) {
    if (! seconds) {
      struct std::tm * desc = std::localtime(&moment);
      if (years)
	desc->tm_mon = 0;
      desc->tm_mday = 1;
      desc->tm_hour = 0;
      desc->tm_min  = 0;
      desc->tm_sec  = 0;
      quant = std::mktime(desc);
    }

    std::time_t temp;
#if DEBUG_LEVEL >= RELEASE
    int cutoff = 10000;
#endif
    while (std::difftime(moment, temp = increment(quant)) > 0) {
      if (quant == temp)
	break;
      quant = temp;
#if DEBUG_LEVEL >= RELEASE
      assert(--cutoff > 0);
#endif
    }
  }

  return quant;
}

std::time_t interval_t::increment(const std::time_t moment)
{
  std::time_t then = moment;

  if (years || months) {
    struct std::tm * desc = std::localtime(&then);

    if (years)
      desc->tm_year += years;

    if (months) {
      desc->tm_mon += months;

      if (desc->tm_mon > 11) {
	desc->tm_year++;
	desc->tm_mon -= 12;
      }
      else if (desc->tm_mon < 0) {
	desc->tm_year--;
	desc->tm_mon += 12;
      }
    }

    then = std::mktime(desc);
  }

  return then + seconds;
}

static void parse_inclusion_specifier(const std::string& word,
				      std::time_t *	 begin,
				      std::time_t *	 end)
{
  struct std::tm when;

  if (! parse_date_mask(word.c_str(), &when))
    throw interval_expr_error(std::string("Could not parse date mask: ") +
			      word);

  when.tm_hour = 0;
  when.tm_min  = 0;
  when.tm_sec  = 0;

  bool saw_year = true;
  bool saw_mon  = true;

  if (when.tm_year == -1) {
    when.tm_year = now_year;
    saw_year = false;
  }
  if (when.tm_mon == -1) {
    when.tm_mon = 0;
    saw_mon = false;
  }
  if (when.tm_mday == -1)
    when.tm_mday = 1;

  if (begin)
    *begin = std::mktime(&when);
  if (end)
    *end   = interval_t(0, saw_mon ? 1 : 0,
			saw_year ? 1 : 0).increment(*begin);
}

void interval_t::parse(std::istream& in)
{
  std::string word;

  while (! in.eof()) {
    in >> word;
    if (word == "every") {
      in >> word;
      if (std::isdigit(word[0])) {
	int quantity = std::atol(word.c_str());
	in >> word;
	if (word == "days")
	  seconds = 86400 * quantity;
	else if (word == "weeks")
	  seconds = 7 * 86400 * quantity;
	else if (word == "months")
	  months = quantity;
	else if (word == "quarters")
	  months = 3 * quantity;
	else if (word == "years")
	  years = quantity;
      }
      else if (word == "day")
	seconds = 86400;
      else if (word == "week")
	seconds = 7 * 86400;
      else if (word == "monthly")
	months = 1;
      else if (word == "quarter")
	months = 3;
      else if (word == "year")
	years = 1;
    }
    else if (word == "daily")
      seconds = 86400;
    else if (word == "weekly")
      seconds = 7 * 86400;
    else if (word == "biweekly")
      seconds = 14 * 86400;
    else if (word == "monthly")
      months = 1;
    else if (word == "bimonthly")
      months = 2;
    else if (word == "quarterly")
      months = 3;
    else if (word == "yearly")
      years = 1;
    else if (word == "this" || word == "last" || word == "next") {
      std::string type	   = word;
      bool	  mon_spec = false;
      char	  buf[32];

      if (! in.eof())
	in >> word;
      else
	word = "month";

      if (word == "month") {
	std::strftime(buf, 31, "%B", std::localtime(&now));
	word = buf;
	mon_spec = true;
      }
      else if (word == "year") {
	std::strftime(buf, 31, "%Y", std::localtime(&now));
	word = buf;
      }

      parse_inclusion_specifier(word, &begin, &end);

      if (type == "last") {
	if (mon_spec) {
	  begin = interval_t(0, -1, 0).increment(begin);
	  end   = interval_t(0, -1, 0).increment(end);
	} else {
	  begin = interval_t(0, 0, -1).increment(begin);
	  end   = interval_t(0, 0, -1).increment(end);
	}
      }
      else if (type == "next") {
	if (mon_spec) {
	  begin = interval_t(0, 1, 0).increment(begin);
	  end   = interval_t(0, 1, 0).increment(end);
	} else {
	  begin = interval_t(0, 0, 1).increment(begin);
	  end   = interval_t(0, 0, 1).increment(end);
	}
      }
    }
    else if (word == "in") {
      in >> word;
      parse_inclusion_specifier(word, &begin, &end);
    }
    else if (word == "from") {
      in >> word;
      if (! parse_date(word.c_str(), &begin))
	throw interval_expr_error("Could not parse 'from' date");
      if (! in.eof())
	in >> word;
    }
    else if (word == "to") {
      in >> word;
      if (! parse_date(word.c_str(), &end))
	throw interval_expr_error("Could not parse 'to' date");
    }
    else {
      parse_inclusion_specifier(word, &begin, &end);
    }
  }
}

bool parse_date_mask(const char * date_str, struct std::tm * result)
{
  for (const char ** f = formats; *f; f++) {
    memset(result, INT_MAX, sizeof(struct std::tm));
    if (strptime(date_str, *f, result))
      return true;
  }
  return false;
}

bool parse_date(const char * date_str, std::time_t * result, const int year)
{
  struct std::tm when;

  if (! parse_date_mask(date_str, &when))
    return false;

  when.tm_hour = 0;
  when.tm_min  = 0;
  when.tm_sec  = 0;

  if (when.tm_year == -1)
    when.tm_year = ((year == -1) ? now_year : (year - 1900));

  if (when.tm_mon == -1)
    when.tm_mon = 0;

  if (when.tm_mday == -1)
    when.tm_mday = 1;

  *result = std::mktime(&when);

  return true;
}

bool quick_parse_date(char * date_str, std::time_t * result)
{
  int year = -1, month = -1, day, num = 0;

  for (char * p = date_str; *p; p++) {
    if (*p == '/' || *p == '-' || *p == '.') {
      if (year == -1)
	year = num;
      else
	month = num;
      num = 0;
    }
    else if (*p < '0' || *p > '9') {
      return false;
    }
    else {
      num *= 10;
      num += *p - '0';
    }
  }

  day = num;

  if (month == -1) {
    month = year;
    year  = -1;
  }

  if (base == -1 || year != base_year) {
    struct std::tm when;
    std::memset(&when, 0, sizeof(when));

    base_year    = year == -1 ? now_year + 1900 : year;
    when.tm_year = year == -1 ? now_year : year - 1900;
    when.tm_mday = 1;

    base = std::mktime(&when);
  }

  *result = base;

  --month;
  while (--month >= 0) {
    *result += month_days[month] * 24 * 60 * 60;
    if (month == 1 && year % 4 == 0 && year != 2000) // february in leap years
      *result += 24 * 60 * 60;
  }

  if (--day)
    *result += day * 24 * 60 * 60;

  return true;
}

} // namespace ledger

#ifdef USE_BOOST_PYTHON

#include <boost/python.hpp>

using namespace boost::python;
using namespace ledger;

unsigned int interval_len(interval_t& interval)
{
  int periods = 1;
  std::time_t when = interval.first();
  while (interval.end && when < interval.end) {
    when = interval.increment(when);
    if (when < interval.end)
      periods++;
  }
  return periods;
}

std::time_t interval_getitem(interval_t& interval, int i)
{
  static std::time_t last_index = 0;
  static std::time_t last_moment = 0;

  if (i == 0) {
    last_index = 0;
    last_moment = interval.first();
  }
  else {
    last_moment = interval.increment(last_moment);
    if (interval.end && last_moment >= interval.end) {
      PyErr_SetString(PyExc_IndexError, "Index out of range");
      throw_error_already_set();
    }
  }
  return last_moment;
}

void export_datetime()
{
  class_< interval_t >
    ("Interval", init<optional<int, int, int, std::time_t, std::time_t> >())
    .def(init<std::string>())
    .def(! self)

    .def_readwrite("years", &interval_t::years)
    .def_readwrite("months", &interval_t::months)
    .def_readwrite("seconds", &interval_t::seconds)
    .def_readwrite("begin", &interval_t::begin)
    .def_readwrite("end", &interval_t::end)

    .def("__len__", interval_len)
    .def("__getitem__", interval_getitem)

    .def("increment", &interval_t::increment)
    ;
}

#endif // USE_BOOST_PYTHON
