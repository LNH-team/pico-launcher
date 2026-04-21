#pragma once
#include <libtwl/math/mathDiv.h>

template <typename T, u32 FractionBits>
class fixed
{
protected:
    T _value;

    constexpr fixed(T rawValue, int dummy)
        : _value(rawValue) { }

public:
    constexpr fixed() { }

    template <typename T2>
    constexpr fixed(fixed<T2, FractionBits> value)
        : _value(value.GetRawValue()) { }

    template <typename T2, u32 OtherFractionBits>
    explicit constexpr fixed(fixed<T2, OtherFractionBits> value)
    {
        if (FractionBits == OtherFractionBits)
            _value = value.GetRawValue();
        else if (OtherFractionBits < FractionBits)
            _value = value.GetRawValue() << (FractionBits - OtherFractionBits);
        else
        {
            const T2 rounding = (T2)(1ULL << ((OtherFractionBits - FractionBits) - 1));
            const int shift = OtherFractionBits - FractionBits;
            _value = (value.GetRawValue() + rounding) >> shift;
        }
    }

    constexpr fixed(int value)
        : _value(value << FractionBits) { }

    constexpr fixed(s64 value)
        : _value(value << FractionBits) { }

    constexpr fixed(float value)
        : _value(std::round(value * (1 << FractionBits))) { }

    constexpr fixed(double value)
        : _value(std::round(value * (1 << FractionBits))) { }

    constexpr fixed(long double value)
        : _value(std::round(value * (1 << FractionBits))) { }

    constexpr T Int() const
    {
        return _value >> FractionBits;
    }

    constexpr T GetRawValue() const { return _value; }
};

template <u32 FractionBits>
class fix32;

template <u32 FractionBits>
class fix64;

template <u32 FractionBits>
class fix16 : public fixed<s16, FractionBits>
{
    constexpr fix16(s16 rawValue, int dummy)
        : fixed<s16, FractionBits>(rawValue, dummy) { }

public:
    constexpr fix16() { }

    constexpr fix16(const fix16<FractionBits>& value)
        : fixed<s16, FractionBits>(value) { }

    template <typename T2, u32 OtherFractionBits>
    explicit constexpr fix16(const fixed<T2, OtherFractionBits>& value)
        : fixed<s16, FractionBits>(value) { }

    constexpr fix16(s16 value)
        : fixed<s16, FractionBits>(value) { }

    constexpr fix16(int value)
        : fixed<s16, FractionBits>(value) { }

    constexpr fix16(float value)
        : fixed<s16, FractionBits>(value) { }

    constexpr fix16(double value)
        : fixed<s16, FractionBits>(value) { }

    constexpr fix16(long double value)
        : fixed<s16, FractionBits>(value) { }

    constexpr bool operator<(const fix16& other) const
    {
        return this->_value < other._value;
    }

    constexpr bool operator<=(const fix16& other) const
    {
        return this->_value <= other._value;
    }

    constexpr bool operator>(const fix16& other) const
    {
        return this->_value > other._value;
    }

    constexpr bool operator>=(const fix16& other) const
    {
        return this->_value >= other._value;
    }

    constexpr fix16 operator-() const
    {
        return FromRawValue(-this->_value);
    }

    constexpr fix16 operator+(const fix16& other) const
    {
        return FromRawValue(this->_value + other._value);
    }

    template <typename TOther>
    constexpr fix16 operator+(TOther other) const
    {
        return FromRawValue(this->_value + fix16(other)._value);
    }

    template <typename TLhs>
    constexpr friend fix16 operator+(TLhs lhs, const fix16& rhs)
    {
        return FromRawValue(fix16(lhs)._value + rhs._value);
    }

    constexpr fix16 operator-(const fix16& other) const
    {
        return FromRawValue(this->_value - other._value);
    }

    template <typename TOther>
    constexpr fix16 operator-(TOther other) const
    {
        return FromRawValue(this->_value - fix16(other)._value);
    }

    template <typename TLhs>
    constexpr friend fix16 operator-(TLhs lhs, fix16 rhs)
    {
        return FromRawValue(fix16(lhs)._value - rhs._value);
    }

    constexpr fix16 operator<<(int rhs) const
    {
        return FromRawValue(this->_value << rhs);
    }

    constexpr fix16 operator>>(int rhs) const
    {
        return FromRawValue(this->_value >> rhs);
    }

    constexpr fix16 Abs() const
    {
        return FromRawValue(std::abs(this->_value));
    }

    constexpr fix16 Clamp(fix16 min, fix16 max) const
    {
        if (this->_value < min._value)
        {
            return min;
        }
        else if (this->_value > max._value)
        {
            return max;
        }
        else
        {
            return *this;
        }
    }

    template <u32 OtherFractionBits>
    constexpr fix32<FractionBits + OtherFractionBits> LongMul(const fix16<OtherFractionBits>& other) const
    {
        return fix32<FractionBits + OtherFractionBits>::FromRawValue(this->_value * other.GetRawValue());
    }

    template <u32 OtherFractionBits>
    constexpr fix64<FractionBits + OtherFractionBits> LongMul(const fix32<OtherFractionBits>& other) const
    {
        return fix64<FractionBits + OtherFractionBits>::FromRawValue(this->_value * (s64)other._value);
    }

    template <u32 OtherFractionBits>
    constexpr fix16 operator*(const fix16<OtherFractionBits>& other) const
    {
        return fix16(LongMul(other));
    }

    template <u32 OtherFractionBits>
    constexpr fix16 operator*(const fix32<OtherFractionBits>& other) const
    {
        return fix16(LongMul(other));
    }

    constexpr fix16 operator*(s16 other) const
    {
        return FromRawValue(this->_value * other);
    }

    constexpr friend fix16 operator*(s16 lhs, const fix16& rhs)
    {
        return FromRawValue(lhs * rhs.GetRawValue());
    }

    template <u32 OtherFractionBits>
    constexpr fix16 operator/(const fix16<OtherFractionBits>& other) const
    {
        s32 lhs = (s32)this->_value << 16;
        s32 rhs = other.GetRawValue();
        s32 divResult;
        if (std::is_constant_evaluated())
            divResult = lhs / rhs;
        else
            divResult = math_div32(lhs, rhs);
        return fix16(fix32<FractionBits + 16 - OtherFractionBits>::FromRawValue(divResult));
    }

    static constexpr fix16 FromRawValue(s16 value)
    {
        return fix16(value, 0);
    }
};

template <u32 FractionBits>
class fix32 : public fixed<int, FractionBits>
{
    constexpr fix32(int rawValue, int dummy)
        : fixed<int, FractionBits>(rawValue, dummy) { }

public:
    constexpr fix32() { }

    constexpr fix32(const fix16<FractionBits>& value)
        : fixed<int, FractionBits>(value) { }

    constexpr fix32(const fix32<FractionBits>& value)
        : fixed<int, FractionBits>(value) { }

    template <typename T2, u32 OtherFractionBits>
    explicit constexpr fix32(const fixed<T2, OtherFractionBits>& value)
        : fixed<int, FractionBits>(value) { }

    constexpr fix32(int value)
        : fixed<int, FractionBits>(value) { }

    constexpr fix32(float value)
        : fixed<int, FractionBits>(value) { }

    constexpr fix32(double value)
        : fixed<int, FractionBits>(value) { }

    constexpr fix32(long double value)
        : fixed<int, FractionBits>(value) { }

    constexpr bool operator==(const fix32& other) const
    {
        return this->_value == other._value;
    }

    constexpr bool operator!=(const fix32& other) const
    {
        return this->_value != other._value;
    }

    constexpr bool operator<(const fix32& other) const
    {
        return this->_value < other._value;
    }

    template <typename TLhs>
    constexpr friend bool operator<(TLhs lhs, const fix32& rhs)
    {
        return fix32(lhs)._value < rhs._value;
    }

    constexpr bool operator<=(const fix32& other) const
    {
        return this->_value <= other._value;
    }

    template <typename TLhs>
    constexpr friend bool operator<=(TLhs lhs, const fix32& rhs)
    {
        return fix32(lhs)._value <= rhs._value;
    }

    constexpr bool operator>(const fix32& other) const
    {
        return this->_value > other._value;
    }

    template <typename TLhs>
    constexpr friend bool operator>(TLhs lhs, const fix32& rhs)
    {
        return fix32(lhs)._value > rhs._value;
    }

    constexpr bool operator>=(const fix32& other) const
    {
        return this->_value >= other._value;
    }

    template <typename TLhs>
    constexpr friend bool operator>=(TLhs lhs, const fix32& rhs)
    {
        return fix32(lhs)._value >= rhs._value;
    }

    constexpr fix32 operator-() const
    {
        return FromRawValue(-this->_value);
    }

    constexpr fix32 operator+(const fix32& other) const
    {
        return FromRawValue(this->_value + other._value);
    }

    template <typename TOther>
    constexpr fix32 operator+(TOther other) const
    {
        return FromRawValue(this->_value + fix32(other)._value);
    }

    template <typename TLhs>
    constexpr friend fix32 operator+(TLhs lhs, const fix32& rhs)
    {
        return FromRawValue(fix32(lhs)._value + rhs._value);
    }

    constexpr fix32 operator-(const fix32& other) const
    {
        return FromRawValue(this->_value - other._value);
    }

    constexpr fix32 operator-(int other) const
    {
        return FromRawValue(this->_value - fix32(other)._value);
    }

    constexpr friend fix32 operator-(int lhs, const fix32& rhs)
    {
        return FromRawValue(fix32(lhs)._value - rhs._value);
    }

    constexpr fix32 operator/(int other) const
    {
        return FromRawValue(this->_value / other);
    }

    constexpr fix32 operator<<(int rhs) const
    {
        return FromRawValue(this->_value << rhs);
    }

    constexpr fix32 operator>>(int rhs) const
    {
        return FromRawValue(this->_value >> rhs);
    }

    constexpr fix32 Abs() const
    {
        return FromRawValue(std::abs(this->_value));
    }

    constexpr fix32 Clamp(fix32 min, fix32 max) const
    {
        if (this->_value < min._value)
        {
            return min;
        }
        else if (this->_value > max._value)
        {
            return max;
        }
        else
        {
            return *this;
        }
    }

    template <u32 OtherFractionBits>
    constexpr fix64<FractionBits + OtherFractionBits> LongMul(const fix16<OtherFractionBits>& other) const
    {
        return fix64<FractionBits + OtherFractionBits>::FromRawValue((s64)this->_value * other.GetRawValue());
    }

    template <u32 OtherFractionBits>
    constexpr fix64<FractionBits + OtherFractionBits> LongMul(const fix32<OtherFractionBits>& other) const
    {
        return fix64<FractionBits + OtherFractionBits>::FromRawValue((s64)this->_value * other.GetRawValue());
    }

    constexpr fix64<FractionBits> LongMul(int other) const
    {
        return fix64<FractionBits>::FromRawValue((s64)this->_value * other);
    }

    constexpr fix64<FractionBits> LongMul(double other) const
    {
        return LongMul(fix32(other));
    }

    template <u32 OtherFractionBits>
    constexpr fix32 operator*(const fix16<OtherFractionBits>& other) const
    {
        return fix32(LongMul(other));
    }

    template <u32 OtherFractionBits>
    constexpr fix32 operator*(const fix32<OtherFractionBits>& other) const
    {
        return fix32(LongMul(other));
    }

    constexpr fix32 operator*(int other) const
    {
        return FromRawValue(this->_value * other);
    }

    constexpr fix32 operator*(double other) const
    {
        return fix32(LongMul(fix32(other)));
    }

    constexpr friend fix32 operator*(int lhs, const fix32& rhs)
    {
        return FromRawValue(lhs * rhs.GetRawValue());
    }

    template <u32 OtherFractionBits>
    constexpr fix32 operator/(const fix16<OtherFractionBits>& other) const
    {
        s64 lhs = (s64)this->_value << 32;
        s32 rhs = other.GetRawValue();
        s64 divResult;
        if (std::is_constant_evaluated())
            divResult = lhs / rhs;
        else
            divResult = math_div6432(lhs, rhs);
        return fix32(fix64<FractionBits + 32 - OtherFractionBits>::FromRawValue(divResult));
    }

    template <u32 OtherFractionBits>
    constexpr fix32 operator/(const fix32<OtherFractionBits>& other) const
    {
        s64 lhs = (s64)this->_value << 32;
        s32 rhs = other.GetRawValue();
        s64 divResult;
        if (std::is_constant_evaluated())
            divResult = lhs / rhs;
        else
            divResult = math_div6432(lhs, rhs);
        return fix32(fix64<FractionBits + 32 - OtherFractionBits>::FromRawValue(divResult));
    }

    static constexpr fix32 FromRawValue(int value)
    {
        return fix32(value, 0);
    }
};

// static constexpr fix32<12> operator""fx(long double value) { return fix32<12>(value); }

template <u32 FractionBits>
class fix64 : public fixed<s64, FractionBits>
{
    constexpr fix64(s64 rawValue, int dummy)
        : fixed<s64, FractionBits>(rawValue, dummy) { }

public:
    constexpr fix64() { }

    template <typename T2>
    constexpr fix64(const fixed<T2, FractionBits>& value)
        : fixed<s64, FractionBits>(value) { }

    template <typename T2, u32 OtherFractionBits>
    explicit constexpr fix64(const fixed<T2, OtherFractionBits>& value)
        : fixed<s64, FractionBits>(value) { }

    constexpr fix64(int value)
        : fixed<s64, FractionBits>(value) { }

    constexpr fix64(s64 value)
        : fixed<s64, FractionBits>(value) { }

    constexpr fix64(float value)
        : fixed<s64, FractionBits>(value) { }

    constexpr fix64(double value)
        : fixed<s64, FractionBits>(value) { }

    constexpr fix64(long double value)
        : fixed<s64, FractionBits>(value) { }

    constexpr bool operator<(const fix64& other) const
    {
        return this->_value < other._value;
    }

    constexpr bool operator<=(const fix64& other) const
    {
        return this->_value <= other._value;
    }

    constexpr bool operator>(const fix64& other) const
    {
        return this->_value > other._value;
    }

    constexpr bool operator>=(const fix64& other) const
    {
        return this->_value >= other._value;
    }

    constexpr fix64 operator+(const fix64& other) const
    {
        return FromRawValue(this->_value + other._value);
    }

    constexpr fix64 operator-(const fix64& other) const
    {
        return FromRawValue(this->_value - other._value);
    }

    static constexpr fix64 FromRawValue(s64 value)
    {
        return fix64(value, 0);
    }
};