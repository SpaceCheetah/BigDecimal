#pragma once
#include <vector>
#include <string>

class BigDecimal {
public:
	BigDecimal() = default;
	//Implicit conversions
	BigDecimal(long long);
	BigDecimal(long);
	BigDecimal(int);
	BigDecimal(short);
	BigDecimal(char);
    BigDecimal(float, int precision = 7);
	BigDecimal(double, int precision = 15);
	BigDecimal(long double, int precision = 15);
	//Explicit conversions
	explicit BigDecimal(const std::string&);
	explicit BigDecimal(const char*);
	//Throws an exception if it can't fit, and truncates everything after the decimal point
	int64_t toInt64();
	//can lose information
	long double toLongDouble();
	//Doesn't lose information, but still shouldn't be implicit
	std::string toString() const;
	//copy and move constructors are implicitly defined to be member-wise, which is fine here
	//member operators
	BigDecimal& operator+=(const BigDecimal&);
	BigDecimal& operator-=(const BigDecimal&);
	BigDecimal& operator*=(const BigDecimal&);
    //Passing by value as copies have to be made anyway
	BigDecimal& operator/=(BigDecimal);
	BigDecimal& operator%=(BigDecimal);
	BigDecimal& operator++(); //prefix
	const BigDecimal operator++(int);//postfix
	BigDecimal& operator--(); //prefix
	const BigDecimal operator--(int); //postfix
	//friend operators (to allow argument symmetry via ADL lookup)
	//First argument is passed by value to prevent having to create a new copy
	friend BigDecimal operator+(BigDecimal, const BigDecimal&);
	friend BigDecimal operator-(BigDecimal, const BigDecimal&);
	friend BigDecimal operator*(BigDecimal, const BigDecimal&);
	friend BigDecimal operator/(BigDecimal, BigDecimal);
	friend BigDecimal operator%(BigDecimal, BigDecimal);
	//== can be default; all member variables have equality, so default equals check is fine
	friend bool operator==(const BigDecimal&, const BigDecimal&) = default;
	//<=> cannot be default, as digits are stored in reverse order
	//<=> is similar to compareTo in Java
	friend std::strong_ordering operator<=>(const BigDecimal&, const BigDecimal&);
    //Stream operators
    friend std::ostream& operator<<(std::ostream&, const BigDecimal&);
    friend std::istream& operator>>(std::istream&, BigDecimal&);
private:
	std::vector<char> digits{};
	bool sign{true};
	int exponent{0};
	//Ensure proper equality:
	// no trailing zeroes or leading zeroes
	// if this == 0 should be true, sign == true, exponent = 0, digits.clear()
	void normalize();
	void constructFromChars(std::string);
	void doAdd(const BigDecimal&);
	void doSubtract(const BigDecimal&);
	static BigDecimal singleDigitMultiply(BigDecimal bd, int digit);
};