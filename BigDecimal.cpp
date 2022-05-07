#include "BigDecimal.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <utility>
#include <cmath>
#include <compare>

BigDecimal::BigDecimal(long long i) {
	sign = i >= 0;
	while (i != 0) {
		digits.push_back(static_cast<char>('0' + std::abs(i % 10)));
		i /= 10;
	}
	normalize();
}

BigDecimal::BigDecimal(long i) : BigDecimal(static_cast<long long>(i)) {}
BigDecimal::BigDecimal(int i) : BigDecimal(static_cast<long long>(i)) {}
BigDecimal::BigDecimal(short i) : BigDecimal(static_cast<long long>(i)) {}
BigDecimal::BigDecimal(char i) : BigDecimal(static_cast<long long>(i)) {}
BigDecimal::BigDecimal(double d, int precision) : BigDecimal(static_cast<long double>(d), precision) {}
BigDecimal::BigDecimal(float d, int precision) : BigDecimal(static_cast<long double>(d), precision) {}

BigDecimal::BigDecimal(long double d, int precision) {
	std::stringstream stream{};
	stream << std::scientific << std::setprecision(precision) << d;
	constructFromChars(stream.str());
}

BigDecimal::BigDecimal(const std::string& str) {
	constructFromChars(str);
}

BigDecimal::BigDecimal(const char* chars) {
	constructFromChars(std::string(chars));
}

void BigDecimal::constructFromChars(std::string str) {
	if (str.empty()) {
		return;
	}
	auto begin = str.begin();
	auto end = str.end();
	if (str.at(0) == '-' || str.at(0) == '+') {
		sign = str.at(0) == '+';
		begin++;
		if (begin == end) {
			throw std::invalid_argument(std::string("\"") + str + "\" is not a valid decimal");
		}
	}
	auto pointIter = std::find(begin, end, '.');
	auto eIter = std::find_if(begin, end, [](char c) {return c == 'e' || c == 'E'; });
	if (eIter < pointIter && pointIter != end) {
		throw std::invalid_argument(std::string("\"") + str + "\" is not a valid decimal");
	}
	if (eIter != end) {
		exponent = std::stoi(std::string(eIter + 1, end));
		end = eIter;
	}
	if (!(pointIter == end || pointIter > eIter)) {
		//Remove trailing zeroes after decimal point
		while (*(end - 1) == '0')
			--end;
		exponent -= static_cast<int>(end - pointIter - 1);
		end = std::shift_left(pointIter, end, 1);
	}
    if(end == begin) {
        throw std::invalid_argument(std::string("\"") + str + "\" is not a valid decimal");
    }
	auto iter = end - 1;
	while (true) {
		if (*iter < '0' || *iter > '9') throw std::invalid_argument(std::string("\"") + str + "\" is not a valid decimal");
		digits.push_back(*iter);
		if (iter == begin) break;
		iter--;
	}
	normalize();
}

void BigDecimal::normalize() {
	//Digits are stored least significant to most, so leading zeroes are significant, trailing zeroes are not
	//Remove trailing zeroes
	for (int i = static_cast<int>(digits.size()) - 1; i >= 0 && digits.at(i) == '0'; i--) {
		digits.pop_back();
	}
	//Ensure equality at 0
	if (digits.empty()) {
		sign = true;
		exponent = 0;
		return;
	}
	//Remove leading zeroes
	for (auto iter = digits.begin(); iter != digits.end() && *iter == '0'; iter = digits.erase(iter)) {
		exponent++;
	}
}

namespace {
	long long longLongE(char digit, int pos) {
		long long result = digit - '0';
		if (result == 0) return 0;
		for (int i = 0; i < pos; i++) {
			result *= 10;
		}
		return result;
	}
}

int64_t BigDecimal::toInt64() {
	if (digits.size() + exponent > 19) {
		if (sign)
			throw std::overflow_error("BigDecimal can't fit in long long");
		else throw std::underflow_error("BigDecimal can't fit in long long");
	}
	long long result = 0;
	int pos = exponent;
	if (sign) {
		for (char digit : digits) {
			if (pos >= 0) {
				long long toAdd = longLongE(digit, pos);
				if (toAdd > std::numeric_limits<long long>::max() - result) {
					throw std::overflow_error("BigDecimal can't fit in long long");
				}
				else result += toAdd;
			}
			pos++;
		}
	}
	else {
		for (char digit : digits) {
			if (pos >= 0) {
				long long toSubtract = longLongE(digit, pos);
				if (-1 * toSubtract < std::numeric_limits<long long>::min() - result) {
					throw std::underflow_error("BigDecimal can't fit in long long");
				}
				else result -= toSubtract;
			}
			pos++;
		}
	}
	return result;
}

long double BigDecimal::toLongDouble() {
	int pos = exponent;
	long double result = 0;
	for (char digit : digits) {
		result += (digit - '0') * std::pow(10, pos);
		pos++;
	}
	return result;
}

std::string BigDecimal::toString() const {
	if (digits.empty()) {
		return "0";
	}
	std::string result{};
	int offset = 0;
	if (!sign) {
		result += "-";
		offset = 1;
	}
	result += std::string_view(digits.begin(), digits.end());
	std::reverse(result.begin() + offset, result.end());
	if (exponent < 0) {
		int pointPos = std::abs(exponent);
		if (pointPos == digits.size()) {
			result.insert(offset, "0.");
		}
		else if (pointPos > digits.size()) {
            if(offset + 1 != result.size())
			    result.insert(offset + 1, ".");
			result += "e-";
			result += std::to_string(pointPos - digits.size() + 1);
		}
		else {
			result.insert(offset + digits.size() - pointPos, ".");
		}
	}
	else if (exponent != 0) {
		result += "e+";
		result += std::to_string(exponent);
	}
	return result;
}

BigDecimal& BigDecimal::operator+=(const BigDecimal& bd) {
	if (this->sign == bd.sign) {
		doAdd(bd);
	}
	else {
		doSubtract(bd);
	}
	return *this;
}

BigDecimal& BigDecimal::operator-=(const BigDecimal& bd) {
	if (this->sign == bd.sign) {
		doSubtract(bd);
	}
	else {
		doAdd(bd);
	}
	return *this;
}

BigDecimal BigDecimal::singleDigitMultiply(BigDecimal bd, int digit) {
	bd.sign = true;
	bd.exponent = 0;
	int carry = 0;
	for (char& c : bd.digits) {
		int elem = (c - '0') * digit + carry;
		c = static_cast<char>(elem % 10 + '0');
		carry = elem / 10;
	}
	for (; carry > 0; carry /= 10) {
		bd.digits.push_back(static_cast<char>(carry % 10 + '0'));
	}
	bd.normalize();
	return bd;
}

BigDecimal& BigDecimal::operator*=(const BigDecimal& bd) {
	bool newSign = sign == bd.sign;
	int newExponent = exponent + bd.exponent;
	const BigDecimal* bd1;
	const std::vector<char>* d2;
	if (digits.size() > bd.digits.size()) {
		bd1 = this;
		d2 = &bd.digits;
	}
	else {
		bd1 = &bd;
		d2 = &digits;
	}
	if (d2->empty()) {
		digits.clear();
		normalize();
		return *this;
	}
	BigDecimal total = 0;
	for (int i = 0; i < d2->size(); i++) {
		BigDecimal row = singleDigitMultiply(*bd1, d2->at(i) - '0');
		row.exponent += i;
		total += row;
	}
	*this = total;
	sign = newSign;
	exponent = newExponent + total.exponent;
	normalize();
	return *this;
}

//Dividing in decimal is quite slow; probably the slowest operation implemented
BigDecimal& BigDecimal::operator/=(BigDecimal bd) {
	BigDecimal result{};
	BigDecimal remainder{};
    int exponentResult = exponent - bd.exponent;
    bd.exponent = 0;
	int maxDigits = static_cast<int>(std::max(digits.size(), bd.digits.size())) + 20;
	//Long division starts with the most significant digit, so need to iterate in reverse
	int i = static_cast<int>(digits.size()) - 1;
	for (; i >= 0; i--) {
		char c = digits[i];
		remainder = remainder * 10 + (c - '0');
		BigDecimal bdCpy = bd;
		BigDecimal mult = 0;
		while (bdCpy <= remainder) {
			bdCpy += bd;
			++mult;
		}
		bdCpy -= bd;
		remainder -= bdCpy;
		mult.exponent = i;
		result += mult;
	}
	while (result.digits.size() < maxDigits && remainder != 0) {
		remainder *= 10;
		BigDecimal bdCpy = bd;
		BigDecimal mult = 0;
		while (bdCpy <= remainder) {
			bdCpy += bd;
			++mult;
		}
		bdCpy -= bd;
		remainder -= bdCpy;
		mult.exponent = i;
		result += mult;
		i--;
	}
	result.exponent += exponentResult;
	*this = result;
	normalize();
	return *this;
}

BigDecimal& BigDecimal::operator%=(BigDecimal bd) {
    bd.sign = true;
    bool signTemp = sign;
    sign = true;
    std::strong_ordering cmp = *this <=> bd;
    if(cmp == std::strong_ordering::less) {
        sign = signTemp;
        return *this;
    }
    else if(cmp == std::strong_ordering::equal) {
        *this = BigDecimal{};
        return *this;
    }
	BigDecimal remainder{};
	int exponentChange = 0;
	if (exponent > 0) {
		digits.insert(digits.begin(), exponent, '0');
	}
	else {
		exponentChange = std::abs(exponent);
		bd.exponent += exponentChange;
	}
	exponent = 0;
	int i = static_cast<int>(digits.size()) - 1;
	for (; i >= 0; i--) {
		char c = digits[i];
		remainder = remainder * 10 + (c - '0');
		BigDecimal bdMult = bd;
		while (bdMult <= remainder) {
			bdMult += bd;
		}
		remainder -= bdMult - bd;
	}
	*this = remainder;
	exponent -= exponentChange;
    sign = signTemp;
	return *this;
}

BigDecimal& BigDecimal::operator++() {
	return *this += 1LL;
}

const BigDecimal BigDecimal::operator++(int) {
	BigDecimal copy = *this;
	*this += 1LL;
	return copy;
}

BigDecimal& BigDecimal::operator--() {
	return *this -= 1LL;
}

const BigDecimal BigDecimal::operator--(int) {
	BigDecimal copy = *this;
	*this -= 1LL;
	return copy;
}

BigDecimal operator+(BigDecimal lhs, const BigDecimal& rhs) {
	return lhs += rhs;
}

BigDecimal operator-(BigDecimal lhs, const BigDecimal& rhs) {
	return lhs -= rhs;
}

BigDecimal operator*(BigDecimal lhs, const BigDecimal& rhs) {
	return lhs *= rhs;
}

BigDecimal operator/(BigDecimal lhs, BigDecimal rhs) {
	return lhs /= std::move(rhs);
}

BigDecimal operator%(BigDecimal lhs, BigDecimal rhs) {
	return lhs %= std::move(rhs);
}

std::strong_ordering operator<=>(const BigDecimal& lhs, const BigDecimal& rhs) {
	if (lhs.sign == rhs.sign) {
		std::strong_ordering cmp{};
		int lhsMaxDigit = static_cast<int>(lhs.digits.size()) + lhs.exponent;
		int rhsMaxDigit = static_cast<int>(rhs.digits.size()) + rhs.exponent;
		if (lhsMaxDigit == rhsMaxDigit) {
			std::vector<char> lhsCpy = lhs.digits;
			std::vector<char> rhsCpy = rhs.digits;
			std::reverse(lhsCpy.begin(), lhsCpy.end());
			std::reverse(rhsCpy.begin(), rhsCpy.end());
			cmp = std::lexicographical_compare_three_way(lhsCpy.begin(), lhsCpy.end(), rhsCpy.begin(), rhsCpy.end());
		}
		else {
			cmp = lhsMaxDigit <=> rhsMaxDigit;
		}
		if (lhs.sign)
			return cmp;
		else {
			if (cmp == std::strong_ordering::less) return std::strong_ordering::greater;
			else return std::strong_ordering::less;
		}
	}
	else return lhs.sign <=> rhs.sign;
}

std::ostream& operator<<(std::ostream& out, const BigDecimal& bd) {
    out << bd.toString();
    return out;
}

std::istream& operator>>(std::istream& in, BigDecimal& bd) {
    std::string str;
    in >> str;
    bd = {};
    try {
        bd.constructFromChars(str);
    } catch(const std::invalid_argument& e) {
        in.setstate(std::ios::failbit);
    }
    return in;
}

void BigDecimal::doAdd(const BigDecimal& bd) {
	if (exponent > bd.exponent) {
		digits.insert(digits.begin(), exponent - bd.exponent, '0');
		exponent = bd.exponent;
	}
	int align = bd.exponent - exponent;
    if(align > digits.size()) {
        digits.insert(digits.end(), align - digits.size(), '0');
    }
	bool carry = false;
	int numDigits = std::min(static_cast<int>(digits.size()) - align,static_cast<int>(bd.digits.size()));
	int i = 0;
	for (; i < numDigits; i++) {
		digits.at(i + align) += bd.digits.at(i) - '0' + static_cast<char>(carry);
		if (digits.at(i + align) > '9') {
			carry = true;
			digits.at(i + align) -= 10;
		}
		else carry = false;
	}
	for (; i < bd.digits.size(); i++) {
		digits.push_back(bd.digits.at(i) + static_cast<char>(carry));
		if (digits.at(digits.size() - 1) > '9') {
			carry = true;
			digits.at(digits.size() - 1) -= 10;
		}
		else carry = false;
	}
	for (; carry != 0 && i < digits.size() - align; i++) {
		digits.at(i + align) += static_cast<char>(carry);
		if (digits.at(i + align) > '9') {
			carry = digits.at(i + align) - '9';
			digits.at(i + align) -= 10;
		}
		else carry = false;
	}
	if (carry != 0) {
		digits.push_back('1');
	}
	normalize();
}

void BigDecimal::doSubtract(const BigDecimal& bd) {
	std::strong_ordering cmp = *this <=> bd;
	if (cmp == std::strong_ordering::equal) {
		digits.clear();
	}
	else if ((cmp == std::strong_ordering::greater && sign) || (cmp == std::strong_ordering::less && !sign)) {
		if (exponent > bd.exponent) {
			digits.insert(digits.begin(), exponent - bd.exponent, '0');
			exponent = bd.exponent;
		}
		int align = bd.exponent - exponent;
		int numDigits = static_cast<int>(std::min(digits.size() - align, bd.digits.size()));
		for (int i = 0; i < numDigits; i++) {
			digits.at(i + align) -= bd.digits.at(i) - '0';
			if (digits.at(i + align) < '0') {
				digits.at(i + 1 + align)--; //this is allowed, since already know that this > bd
				digits.at(i + align) += 10;
			}
		}
		for (int i = numDigits + align; i < digits.size() - 1 && digits.at(i) < '0'; i++) {
			digits.at(i) += 10;
			digits.at(i + 1)--;
		}
		if (digits.at(digits.size() - 1) < '0') { //Something went terribly wrong
			throw std::logic_error("Invalid digit state");
		}
	}
	else {
		*this = bd - *this; //Creates a copy, but copies are still pretty cheap
		sign = !sign;
	}
	normalize();
}