#if defined(USE_BOOST_KARMA)
#include <boost/spirit/include/karma.hpp>
namespace karma = boost::spirit::karma;
#endif

namespace bprinter{
#if defined(USE_BOOST_KARMA)
template<typename T> void TablePrinter::OutputDecimalNumber(T input){
  *out_stream_ << karma::format(
                 karma::maxwidth(column_widths_.at(j_))[
                   karma::right_align(column_widths_.at(j_))[
                     karma::double_
                   ]
                 ], input
               );

  if (j_ == get_num_columns()-1){
    *out_stream_ << "|\n";
    i_ = i_ + 1;
    j_ = 0;
  } else {
    *out_stream_ << separator_;
    j_ = j_ + 1;
  }
}
#else
template<typename T> void TablePrinter::OutputDecimalNumber(T input){
  // If we cannot handle this number, indicate so
  const auto width = column_widths_.at(j_);

  if (input < 10*(width-1) || input > 10*width){

    std::ostringstream string_out;
    string_out << std::fixed
	           << std::setprecision(width)
	           << std::setw(width)
	           << input;

    auto string_rep = string_out.str();
    string_rep[width - 1] = '*';

    *out_stream_ << string_rep;
   
	std::string_view string_to_print(string_rep);
    *out_stream_ << string_to_print.substr(0, width);
  } else {

    // determine what precision we need
    int precision = column_widths_.at(j_) - 1; // leave room for the decimal point
    if (input < 0)
      --precision; // leave room for the minus sign

    // leave room for digits before the decimal?
    if (input < -1 || input > 1){
      int num_digits_before_decimal = 1 + (int)log10(std::abs(input));
      precision -= num_digits_before_decimal;
    }
    else
      precision --; // e.g. 0.12345 or -0.1234

    if (precision < 0)
      precision = 0; // don't go negative with precision

    *out_stream_ << std::setiosflags(std::ios::fixed)
                 << std::setprecision(precision)
                 << std::setw(column_widths_.at(j_))
                 << input;
  }

  if (j_ == get_num_columns()-1){
    *out_stream_ << "|\n";
    i_ = i_ + 1;
    j_ = 0;
  } else {
    *out_stream_ << separator_;
    j_ = j_ + 1;
  }
}
#endif //USE_BOOST_KARMA
}
