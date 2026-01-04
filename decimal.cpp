//消除GCC/Clang的分节处理警告
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#include <iostream>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <vector>
#include <limits>
#include <climits>
#include <regex>

//Now fixing
/*Now fixing:
* Multiple instances matching argument list
* Template function for construct function
* At head of main() function
* Support for literal constant
* Support for keyword "auto"
*/

//Update log
/* Update log
* 2025/12/24 23:55 Fixed issue: lost accuracy when passing argument to operator/.
* 2025/12/25 00:15 Added template for constructed function for float type.
* 2025/12/25 00:41 Partly fixed compile errors for multiple instances matching argument list.
* 2025/12/25 17:40 Added operator<< and operator>>, but not considered performance.
* 2025/12/26 01:16 Added definition of warning macro and considered multiple compilers, 
                   but not fully confirmed whether warnings would appear.
* 2025/12/27 00:58 Partly supported for literal constants, however scientific and binary
                   are temporarily excluded, which could be seen on the error list.
* 2026/01/01 01:37 Considered the removement in case the default constructor was contradicted.
* 2026/01/01 01:58 Renamed constructFromSignedFraction to constructFromFraction
                   because of the replacement of std::abs, which leads function overload
                   of unsigned numbers, and would lost accuracy.
* 2026/01/01 02:02 Renamed longToData to longlongToData because the correct type is long long.
*/

#pragma region 警告宏

#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x  // 字符串化操作符

#if defined(_MSC_VER)
    // MSVC: 使用 message pragma，格式化为警告
#define warning(msg) __pragma(message(__FILE__ "(" STRINGIZE(__LINE__) "): warning: " msg))

#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: 使用 GCC warning pragma
    // 需要先将msg转换为字符串字面量
#define warning(msg) _Pragma(GCC warning #msg)

#elif defined(__INTEL_COMPILER)
    // Intel: 类似MSVC
#define warning(msg) __pragma(message(__FILE__ "(" STRINGIZE(__LINE__) "): warning: " msg))

#else
    // 未知编译器：使用静态断言产生警告效果
#define warning(msg) static_assert(true, "warning: " msg)
#endif
#pragma endregion

//主要实现部分
class acc {
private:
    typedef long long maxtype; // 最大数据类型。若为无符号数，则可能下溢
    typedef int8_t digittype; // 数位类型。单个数据乘积极端值为-81~+81，int8_t的值刚好可以覆盖

    struct unit {
        bool isPositive = true;
        maxtype accuracy = 0;  // 小数位数，默认0
        std::deque<int8_t> data;  // 从高位到低位存储所有数字（包括整数和小数部分）
    } entry;

    //构造函数模板定义
    //浮点数构造函数
    template <typename T> 
    void constructFromFraction(T val ,maxtype fixedPrecision/* = 15*/
    /*内部使用，强制要求提供参数*/) {
        entry.isPositive = val >= 0;
        val = val >= 0 ? val : -val;

        // 使用指定精度
        long long multiplier = 1;
        for (maxtype i = 0; i < fixedPrecision; i++) {
            multiplier *= 10;
        }

        // 四舍五入
        long long scaled = static_cast<long long>(static_cast<long long>(val) * multiplier + 0.5);

        // 存储
        longlongToData(scaled);
        entry.accuracy = fixedPrecision;
    }

    //整数构造函数
    template <typename T>
    void constructFromInteger(T val) {
        //没有写maxtype fixedPrecision，因为整数不需要注明精确度
        (void)entry.accuracy;
        entry.isPositive = val >= 0;

        val = val >= 0 ? val : static_cast<T>(-static_cast<std::make_signed_t<T>>(val));
        entry.accuracy = 0;
        maxtype convertedValue = val;//提升数据范围
        longlongToData(val);
    }

public:
    // 默认构造函数，若冲突考虑去除
    acc() {
        entry.isPositive = true;
        entry.accuracy = 0;
        entry.data = { 0 };
    }

    // long long类型构造函数
    acc(long long val) {
        constructFromInteger(val);
    }

    // 整数类型构造函数
    acc(int val) {
        constructFromInteger(val);
    }

    acc(unsigned int val) {
        constructFromInteger(val);
    }

    acc(short val) {
        constructFromInteger(val);
    }

    acc(unsigned short val) {
        constructFromInteger(val);
    }

    acc(char val) {
        constructFromInteger(val);
    }

    acc(unsigned char val) {
        constructFromInteger(val);
    }

    
    

    // 字符串构造函数（支持大整数）
    acc(const std::string& str) {
        parseString(str);
    }

    // C风格字符串构造函数
    acc(const char* str) : acc(std::string(str)) {}

    // 浮点数构造函数
    acc(long double val, maxtype fixedPrecision = 15) {
        constructFromFraction<long double>(val, fixedPrecision);
    }
    acc(double val, maxtype fixedPrecision = 15){
        constructFromFraction<double>(val, fixedPrecision);
    }
    acc(float val, maxtype fixedPrecision = 15) {
        constructFromFraction<float>(val, fixedPrecision);
    }

     

    // 复制构造函数
    acc(const acc& other) = default;

    // 移动构造函数
    acc(acc&& other) noexcept = default;

    // 赋值运算符
    acc& operator=(const acc& other) = default;

    // 从字符串赋值
    acc& operator=(const std::string& str) {
        parseString(str);
        return *this;
    }

    acc& operator=(const char* str) {
        return operator=(std::string(str));
    }

    // 从整数赋值
    acc& operator=(maxtype val) {
        *this = acc(val);
        return *this;
    }

    acc& operator=(int val) {
        return operator=(static_cast<maxtype>(val));
    }

    // 从浮点数赋值
    acc& operator=(long double val) {
        *this = acc(val);
        return *this;
    }

    // 类型转换运算符
    operator maxtype() const {
        maxtype result = 0;
        maxtype multiplier = 1;

        // 只转换整数部分
        maxtype integerDigits = entry.data.size() - entry.accuracy;
        if (integerDigits == 0) return 0;

        // 从低位开始转换
        for (size_t i = 0; i < integerDigits; i++) {
            maxtype idx = entry.data.size() - entry.accuracy - 1 - i;
            if (idx < entry.data.size()) {
                result += static_cast<maxtype>(entry.data[idx]) * multiplier;
                // 检查溢出
                if (multiplier > LLONG_MAX / 10) {
                    break;
                }
                multiplier *= 10;
            }
        }

        if (!entry.isPositive) {
            result = -result;
        }

        return result;
    }

    // 转换为long double
    operator long double() const {
        long double result = 0.0;
        long double multiplier = 1.0;

        // 从低位开始计算
        for (auto it = entry.data.rbegin(); it != entry.data.rend(); ++it) {
            result += static_cast<long double>(*it) * multiplier;
            multiplier *= 10.0;
        }

        // 考虑小数部分
        if (entry.accuracy > 0) {
            result /= std::pow(10.0L, static_cast<long double>(entry.accuracy));
        }

        if (!entry.isPositive) {
            result = -result;
        }

        return result;
    }

    // 转换为字符串
    std::string to_string() const {
        return toString();
    }

    // 加法运算符
    acc operator+(const acc& other) const {
        // 对齐精度
        acc thisCopy = *this;
        acc otherCopy = other;
        synchronizeAccuracy(thisCopy, otherCopy);

        // 符号相同
        if (thisCopy.entry.isPositive == otherCopy.entry.isPositive) {
            acc result;
            result.entry.isPositive = thisCopy.entry.isPositive;
            result.entry.accuracy = thisCopy.entry.accuracy;

            // 从低位开始相加
            std::deque<int8_t> sum;
            int carry = 0;

            size_t maxSize = std::max(thisCopy.entry.data.size(), otherCopy.entry.data.size());

            for (size_t i = 0; i < maxSize || carry > 0; i++) {
                int digit1 = (i < thisCopy.entry.data.size()) ?
                    thisCopy.entry.data[thisCopy.entry.data.size() - 1 - i] : 0;
                int digit2 = (i < otherCopy.entry.data.size()) ?
                    otherCopy.entry.data[otherCopy.entry.data.size() - 1 - i] : 0;

                int sumDigit = digit1 + digit2 + carry;
                carry = sumDigit / 10;
                sumDigit %= 10;

                sum.push_front(static_cast<int8_t>(sumDigit));
            }

            result.entry.data = sum;
            result.removeLeadingZeros();
            return result;
        }

        // 符号不同，转换为减法
        if (thisCopy.entry.isPositive) {
            return thisCopy - (-otherCopy);
        }
        else {
            return otherCopy - (-thisCopy);
        }
    }

    // 减法运算符
    acc operator-(const acc& other) const {
        // 对齐精度
        acc thisCopy = *this;
        acc otherCopy = other;
        synchronizeAccuracy(thisCopy, otherCopy);

        // 符号不同
        if (thisCopy.entry.isPositive != otherCopy.entry.isPositive) {
            acc result = thisCopy + (-otherCopy);
            return result;
        }

        // 符号相同
        bool isThisLarger = compareAbsolute(*this, other) >= 0;
        const acc& larger = isThisLarger ? thisCopy : otherCopy;
        const acc& smaller = isThisLarger ? otherCopy : thisCopy;

        acc result;
        result.entry.isPositive = (isThisLarger && thisCopy.entry.isPositive) ||
            (!isThisLarger && !thisCopy.entry.isPositive);
        result.entry.accuracy = thisCopy.entry.accuracy;

        // 从低位开始相减
        std::deque<int8_t> diff;
        int borrow = 0;

        size_t maxSize = larger.entry.data.size();

        for (size_t i = 0; i < maxSize; i++) {
            int digit1 = (i < larger.entry.data.size()) ?
                larger.entry.data[larger.entry.data.size() - 1 - i] : 0;
            int digit2 = (i < smaller.entry.data.size()) ?
                smaller.entry.data[smaller.entry.data.size() - 1 - i] : 0;

            int diffDigit = digit1 - digit2 - borrow;

            if (diffDigit < 0) {
                diffDigit += 10;
                borrow = 1;
            }
            else {
                borrow = 0;
            }

            diff.push_front(static_cast<int8_t>(diffDigit));
        }

        result.entry.data = diff;
        result.removeLeadingZeros();

        // 如果结果为0，确保符号为正
        if (result.entry.data.size() == 1 && result.entry.data[0] == 0) {
            result.entry.isPositive = true;
        }

        return result;
    }

    // 乘法运算符
    acc operator*(const acc& other) const {
        acc result;
        result.entry.isPositive = !(entry.isPositive ^ other.entry.isPositive);
        result.entry.accuracy = entry.accuracy + other.entry.accuracy;

        size_t len1 = entry.data.size();
        size_t len2 = other.entry.data.size();
        std::vector<int> product(len1 + len2, 0);

        // 模拟手工乘法
        for (int i = (int)len1 - 1; i >= 0; i--) {
            for (int j = (int)len2 - 1; j >= 0; j--) {
                int mul = entry.data[i] * other.entry.data[j];
                int sum = mul + product[i + j + 1];

                product[i + j + 1] = sum % 10;
                product[i + j] += sum / 10;
            }
        }

        // 转换为deque
        result.entry.data.clear();
        for (int digit : product) {
            result.entry.data.push_back(static_cast<int8_t>(digit));
        }

        result.removeLeadingZeros();
        return result;
    }

    // 复合赋值运算符
    acc& operator+=(const acc& other) {
        *this = *this + other;
        return *this;
    }

    acc& operator-=(const acc& other) {
        *this = *this - other;
        return *this;
    }

    acc& operator*=(const acc& other) {
        *this = *this * other;
        return *this;
    }

    acc operator%(const acc& other) const {
        if (other == acc(0)) {
            throw std::runtime_error("Division by zero in modulo operation");
        }

        // 计算 a / b 的整数商（向零取整）
        acc quotient = (maxtype)*this / (maxtype)other;

        // 获取整数部分（向零取整）
        acc truncated = trunc(quotient);

        // 计算余数：a - truncated * b
        acc remainder = *this - truncated * other;

        return remainder;
    }

    // 向零取整的辅助函数
    static acc trunc(const acc& value) {
        acc result = value;

        // 如果 value >= 0，移除小数部分（floor）
        // 如果 value < 0，也移除小数部分，但因为是负数，相当于向上取整
        if (result.getAccuracy() > 0) {
            // 移除小数部分的所有数字
            for (maxtype i = 0; i < result.entry.accuracy; i++) {
                if (!result.entry.data.empty()) {
                    result.entry.data.pop_back();
                }
            }
            result.entry.accuracy = 0;
            result.removeLeadingZeros();
        }

        return result;
    }

    // 除法函数，指定结果精度（小数位数）
    static acc div(const acc& dividend, const acc& divisor, maxtype precision = 20) {
        //除零错误
        if (divisor.isZero()) {
            throw std::runtime_error("Division by zero");
        }

        // 处理符号
        bool resultIsPositive = !(dividend.entry.isPositive ^ divisor.entry.isPositive);

        // 取绝对值进行除法
        acc a = abs(dividend);
        acc b = abs(divisor);

        // 对齐精度
        synchronizeAccuracy(a, b);

        // 移除小数部分，转换为整数除法
        maxtype scaleFactor = a.entry.accuracy;
        for (maxtype i = 0; i < scaleFactor; i++) {
            if (!a.entry.data.empty()) a.entry.data.pop_back();
            if (!b.entry.data.empty()) b.entry.data.pop_back();
        }
        a.entry.accuracy = 0;
        b.entry.accuracy = 0;

        // 执行长除法
        acc result = longDivision(a, b, precision);

        // 恢复符号
        result.entry.isPositive = resultIsPositive;
        if (result.isZero()) {
            result.entry.isPositive = true;
        }

        return result;
    }

    // 除法运算符（默认精度）
    acc operator/(const acc& other) const {
        return div(*this, other, entry.accuracy > other.entry.accuracy ?
            entry.accuracy : other.entry.accuracy);
    }

    // 带精度的除法运算符
    acc operator/(const std::pair<const acc&, maxtype>& params) const {
        const acc& other = params.first;
        maxtype precision = params.second;
        return div(*this, other, precision);
    }

    // 前缀递增运算符
    acc& operator++() {
        *this = *this + acc(1);
        return *this;
    }

    // 后缀递增运算符
    acc operator++(int) {
        acc temp = *this;
        ++(*this);
        return temp;
    }

    // 前缀递减运算符
    acc& operator--() {
        *this = *this - acc(1);
        return *this;
    }

    // 后缀递减运算符
    acc operator--(int) {
        acc temp = *this;
        --(*this);
        return temp;
    }

    acc operator+()const {
        acc result = *this;
        return result;
    }

    // 取反运算符
    acc operator-() const {
        acc result = *this;
        if (!(result.entry.data.size() == 1 && result.entry.data[0] == 0)) {
            result.entry.isPositive = !result.entry.isPositive;
        }
        return result;
    }

    //左移操作符
    acc operator<< (const acc& shiftCount) const {
        //错误处理
        if (shiftCount.entry.accuracy > 0) {
            throw std::domain_error("Shift count must be integer");
        }
        /*
        if(shiftCount.isFraction) message(Converting from decimal)
        */
        acc result = *this;
        for (acc i = 0; i < shiftCount; i++) {
            result *= 2;
        }
        return result;
    }

    //右移操作符
    acc operator>> (const acc&) const {
        acc result = *this;
        result *= 2;
        return result;
    }

    // 比较运算符
    bool operator==(const acc& other) const {
        if (entry.isPositive != other.entry.isPositive) return false;
        acc a = *this;
        acc b = other;
        synchronizeAccuracy(a, b);

        if (a.entry.data.size() != b.entry.data.size()) return false;
        for (size_t i = 0; i < a.entry.data.size(); i++) {
            if (a.entry.data[i] != b.entry.data[i]) return false;
        }
        return true;
    }
    // 删除内置运算符
    template<typename T>
    bool operator==(const T& other) const {
        // 可能需要static_assert或SFINAE限制类型
        return this->value == other;
    }
    //template<typename T>
    //bool operator==(T) const = delete;
    //bool operator==(int other) = delete;


    bool operator!=(const acc& other) const {
        return !(*this == other);
    }

    bool operator<(const acc& other) const {
        if (entry.isPositive != other.entry.isPositive) {
            return !entry.isPositive;  // 负数 < 正数
        }

        int cmp = compareAbsolute(*this, other);
        if (cmp == 0) return false;

        if (entry.isPositive) {
            return cmp < 0;  // 正数：绝对值小的更小
        }
        else {
            return cmp > 0;  // 负数：绝对值大的更小
        }
    }

    bool operator>(const acc& other) const {
        return other < *this;
    }

    bool operator<=(const acc& other) const {
        return !(*this > other);
    }

    bool operator>=(const acc& other) const {
        return !(*this < other);
    }

    // 绝对值函数
    static acc abs(const acc& value) {
        acc result = value;
        result.entry.isPositive = true;
        return result;
    }

    // 获取精度
    maxtype getAccuracy() const {
        return entry.accuracy;
    }

    // 设置精度
    void setAccuracy(maxtype newAccuracy) {
        if (newAccuracy == entry.accuracy) return;

        if (newAccuracy > entry.accuracy) {
            // 增加精度，补零
            for (maxtype i = entry.accuracy; i < newAccuracy; i++) {
                entry.data.push_back(0);
            }
        }
        else {
            // 减少精度，可能需要四舍五入
            maxtype diff = entry.accuracy - newAccuracy;
            for (maxtype i = 0; i < diff; i++) {
                entry.data.pop_back();
            }
        }
        entry.accuracy = newAccuracy;
        removeLeadingZeros();
    }

    // 转换为字符串
    std::string toString() const {
        if (entry.data.empty()) {
            return "0";
        }

        std::string result;

        // 符号
        if (!entry.isPositive && !(entry.data.size() == 1 && entry.data[0] == 0)) {
            result += '-';
        }

        // 整数部分
        maxtype integerDigits = entry.data.size() - entry.accuracy;

        if (integerDigits <= 0) {
            // 没有整数部分
            result += '0';
        }
        else {
            // 确保不越界
            maxtype maxIdx = std::min(integerDigits, static_cast<maxtype>(entry.data.size()));
            for (maxtype i = 0; i < maxIdx; i++) {
                result += static_cast<char>('0' + entry.data[i]);
            }
        }

        // 小数部分
        if (entry.accuracy > 0) {
            result += '.';

            // 确保不越界
            maxtype startIdx = std::min(integerDigits, static_cast<maxtype>(entry.data.size()));
            maxtype endIdx = std::min(startIdx + entry.accuracy, (maxtype)entry.data.size());

            if (startIdx < entry.data.size()) {
                for (maxtype i = startIdx; i < endIdx; i++) {
                    result += static_cast<char>('0' + entry.data[i]);
                }
            }
            else {
                // 如果startIdx越界，添加0
                for (maxtype i = 0; i < entry.accuracy; i++) {
                    result += '0';
                }
            }
        }

        return result;
    }

    // 输出运算符
    friend std::ostream& operator<<(std::ostream& os, const acc& value) {
        os << value.toString();
        return os;
    }

    // 输入运算符
    friend std::istream& operator>>(std::istream& is, acc& value) {
        std::string str;
        is >> str;
        value.parseString(str);
        return is;
    }

    acc min(const acc& a, const acc& b) {
        return a < b ? a : b;
    }

    //转换进制（通用方式）
    std::string convertScale(const acc& value, const acc& scale = 10) {
        
        std::string ret;
    }

    acc convertScale(const std::string str) {
        if (str.empty()) return 0;
        //匹配文本格式下的非标准格式：(数据（数字或字母或它们的混合体）)_十进制下表达的数学进制
        std::regex pattern(R"(^\([0-9a-zA-Z]+\)_[0-9]+$)");
        if (!std::regex_match(str, pattern)) {
            return 0;
        }
        auto textItBegin = str.begin();
        textItBegin++;//跳过前置(符号
        auto textItEnd = str.begin();
        while (*textItEnd != ')') {
            textItEnd++;
        }
        //获取数据部分
        std::string value(textItBegin, textItEnd);
    }

#pragma region 数据类型检查

    //检查是否为十进制字面值常量
    bool isDec(const std::string& str) {
        // 允许逗号（无空格）或空格作为分隔符，但不允许"逗号+空格"
        // 三位数字作为一段分隔符分隔的数字
        std::regex pattern(R"(^[0-9]+(?:(?:[,，][0-9]{3})|(?: [0-9]{3}))*(?:\.[0-9]*)?$|^\.[0-9]+$)");
        return std::regex_match(str, pattern);
    }

    //检查是否为八进制字面值常量
    bool isOct(const std::string& str) {
        std::regex pattern(R"(^0[0-7]+(?:'[0-7]+)*$)");
        return std::regex_match(str, pattern);
    }

    //检查是否为二进制字面值常量
    bool isBin(const std::string& str) {
        std::regex pattern(R"(^0[bB][01]+(?:'[01]+)*$)");
        return std::regex_match(str, pattern);
    }

    //检查是否为十六进制字面值常量
    bool isHex(const std::string& str) {
        std::regex pattern(R"(^0[xX][0-9A-Fa-f]+(?:'[0-9A-Fa-f]+)*$)");
        return std::regex_match(str, pattern);
    }

    //检查是否为十六进制字面值常量
   /* bool isHex(const std::string& str) {
        // 形式：整数部分'整数部分.小数部分'小数部分指数部分
        std::regex pattern(R"(^\d+(?:'\d+)*(\.\d+(?:'\d+)*)?([eE][+-]?\d+(?:'\d+)*)?$)");        return std::regex_match(str, pattern);
        return std::regex_match(str, pattern);
    }*/
#pragma endregion

private:
    //重置所有数据
    void resetData(void) {
        this->entry.isPositive = true;
        this->entry.data = { 0 };
        this->entry.accuracy = 0;
    }

    // 工具函数：将long long转换为数据
    void longlongToData(maxtype val) {
        entry.data.clear();

        if (val == 0) {
            entry.data.push_back(0);
            return;
        }

        while (val > 0) {
            entry.data.push_front(static_cast<int8_t>(val % 10));
            val /= 10;
        }
    }

    // 移除字符串中的分隔符（单引号）
    std::string removeSeparators(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c != '\'') {
                result.push_back(c);
            }
        }
        return result;
    }

    // 工具函数：解析字符串（分发器）
    void parseString(const std::string& str) {
        // 移除所有分隔符
        std::string withoutSeparators = removeSeparators(str);

        if (withoutSeparators.empty()) {
            parseDecimal("0");
            return;
        }

        // 检查科学计数法（优先级最高，因为可能与其他进制结合）
        if (isScientific(withoutSeparators)) {
            parseScientific(withoutSeparators);
        }
        // 检查二进制
        else if (isBin(str)) {
            parseBinary(str);
        }
        // 检查十六进制
        else if (isHex(str)) {
            parseHexadecimal(str);
        }
        // 检查八进制
        else if (isOct(str)) {
            parseOctal(str);
        }
        // 检查十进制
        else if (isDec(str)) {
            parseDecimal(withoutSeparators);
        }
        else {
            // 无法识别的格式，设为0
            parseDecimal("0");
        }
    }

    // 工具函数：检查是否为科学计数法
    bool isScientific(const std::string& str) {
        // 查找e或E，且不在开头或结尾
        size_t ePos = str.find_first_of("eE");
        if (ePos == std::string::npos || ePos == 0 || ePos == str.length() - 1) {
            return false;
        }

        // e/E之前必须有至少一个数字
        bool hasDigitBeforeE = false;
        for (size_t i = 0; i < ePos; i++) {
            if (isdigit(str[i]) || str[i] == '.') {
                hasDigitBeforeE = true;
                break;
            }
        }
        if (!hasDigitBeforeE) return false;

        // e/E之后必须是有效的整数（可能带符号）
        size_t afterE = ePos + 1;
        if (str[afterE] == '+' || str[afterE] == '-') {
            afterE++;
        }

        // 检查e/E之后是否有数字
        for (size_t i = afterE; i < str.length(); i++) {
            if (!isdigit(str[i])) return false;
        }
        return true;
    }

    // 处理二进制
    void parseBinary(const std::string& str) {
        // 移除分隔符
        std::string withoutSeparators = removeSeparators(str);

        // 跳过前缀
        std::string binaryDigits = withoutSeparators.substr(2);

        // 转换为十进制
        uint64_t value = 0;
        for (char c : binaryDigits) {
            value = (value << 1) | (static_cast<uint64_t>(c) - static_cast <uint64_t>('0'));
        }

        // 存储到entry
        entry.data.clear();
        entry.accuracy = 0;
        entry.isPositive = true;

        if (value == 0) {
            entry.data.push_back(0);
            return;
        }

        // 转换为十进制数字序列
        while (value > 0) {
            entry.data.push_front(value % 10);
            value /= 10;
        }
    }

    // 处理十进制
    void parseDecimal(const std::string& str) {

        // 处理符号
        size_t start = 0;
        if (str[0] == '-') {
            entry.isPositive = false;
            start = 1;
        }
        else if (str[0] == '+') {
            entry.isPositive = true;
            start = 1;
        }
        else {
            entry.isPositive = true;
        }

        // 查找小数点
        size_t dotPos = str.find('.', start);
        bool hasDot = (dotPos != std::string::npos);

        // 提取数字部分
        std::string digits;
        for (size_t i = start; i < str.length(); i++) {
            if (str[i] == '.') continue;
            if (str[i] >= '0' && str[i] <= '9') {
                digits.push_back(str[i]);
            }
            else {
                break;  // 遇到非数字字符，停止解析
            }
        }

        if (digits.empty()) {
            entry.data.push_back(0);
            return;
        }

        // 移除前导零（保留最后一个零）
        size_t firstNonZero = 0;
        while (firstNonZero < digits.length() - 1 && digits[firstNonZero] == '0') {
            firstNonZero++;
        }
        digits = digits.substr(firstNonZero);

        if (digits.empty()) {
            entry.data.push_back(0);
            return;
        }

        // 转换为数字序列
        for (char c : digits) {
            entry.data.push_back(c - '0');
        }

        // 设置精度
        if (hasDot) {
            // 计算小数位数
            size_t decimalStart = dotPos + 1;
            size_t decimalEnd = str.length();

            // 找到小数部分的结束位置（排除可能的非数字字符）
            for (size_t i = decimalStart; i < str.length(); i++) {
                if (!isdigit(str[i])) {
                    decimalEnd = i;
                    break;
                }
            }

            entry.accuracy = decimalEnd - decimalStart;

            // 移除小数部分尾部的零
            while (entry.accuracy > 0 && entry.data.back() == 0) {
                entry.data.pop_back();
                entry.accuracy--;
            }

            // 特殊处理：如果整数部分为空（如.5），添加前导0
            if (dotPos == start) {
                entry.data.push_front(0);
            }
        }
        else {
            entry.accuracy = 0;
        }

        // 特殊处理：-0的情况
        if (entry.data.size() == 1 && entry.data[0] == 0) {
            entry.isPositive = true;
        }

        // 规范化：移除前导零
        removeLeadingZeros();
    }

    // 处理十六进制
    void parseHexadecimal(const std::string& str) {

        // 跳过前缀
        std::string hexDigits = str.substr(2);

        // 转换为十进制
        uint64_t value = 0;
        for (char c : hexDigits) {
            int digit;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            }
            else if (c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            }
            else if (c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10;
            }
            else {
                digit = 0;
            }
            value = value * 16 + digit;
        }

        // 存储到entry
        entry.data.clear();
        entry.accuracy = 0;
        entry.isPositive = true;

        if (value == 0) {
            entry.data.push_back(0);
            return;
        }

        // 转换为十进制数字序列
        while (value > 0) {
            entry.data.push_front(value % 10);
            value /= 10;
        }
    }

    // 处理八进制
    void parseOctal(const std::string& str) {

        // 跳过前缀0
        std::string octalDigits = str.substr(1);

        // 转换为十进制
        uint64_t value = 0;
        for (char c : octalDigits) {
            value = value * 8 + (static_cast<uint64_t>(c) - static_cast<uint64_t>('0'));
        }

        // 存储到entry
        entry.data.clear();
        entry.accuracy = 0;
        entry.isPositive = true;

        if (value == 0) {
            entry.data.push_back(0);
            return;
        }

        // 转换为十进制数字序列
        while (value > 0) {
            entry.data.push_front(value % 10);
            value /= 10;
        }
    }

    //处理科学记数法
    void parseScientific(const std::string& str) {
        // 找到e/E的位置
        size_t ePos = str.find_first_of("eE");

        // 解析尾数部分
        std::string mantissa = str.substr(0, ePos);
        parseDecimal(mantissa);

        // 解析指数部分
        std::string exponentStr = str.substr(ePos + 1);

        // 解析指数（有符号整数）
        maxtype exponent = 0;
        try {
            exponent = std::stoll(exponentStr);
        }
        catch (...) {
            // 解析失败，设为0
            exponent = 0;
        }

        // 根据指数调整小数点和精度
        if (exponent > 0) {
            // 小数点右移
            for (int64_t i = 0; i < exponent; i++) {
                if (entry.accuracy > 0) {
                    entry.accuracy--;
                }
                else {
                    entry.data.push_back(0);
                }
            }
        }
        else if (exponent < 0) {
            // 小数点左移
            maxtype absExp = -exponent;
            if (absExp > static_cast<int64_t>(entry.data.size())) {
                // 需要补零
                maxtype zerosNeeded = absExp - entry.data.size();
                for (maxtype i = 0; i < zerosNeeded; i++) {
                    entry.data.push_front(0);
                }
                entry.accuracy = entry.data.size();
            }
            else {
                entry.accuracy += absExp;
            }
        }

        // 重新规范化
        removeLeadingZeros();
        removeTrailingZeros();
    }


    // 工具函数：移除前导零
    void removeLeadingZeros() {
        // 只移除整数部分的前导零，保留小数部分
        maxtype integerDigits = entry.data.size() - entry.accuracy;

        while (integerDigits > 1 && entry.data[0] == 0) {
            entry.data.pop_front();
            integerDigits--;
        }

        // 如果所有位都是0，保留一个0
        if (entry.data.empty()) {
            entry.data.push_back(0);
            entry.accuracy = 0;
            entry.isPositive = true;
        }
    }

    // 移除尾随零（针对小数部分）
    void removeTrailingZeros() {
        while (entry.accuracy > 0 && entry.data.size() > 1 && entry.data.back() == 0) {
            entry.data.pop_back();
            entry.accuracy--;
        }
    }

    // 工具函数：同步两个数的精度
    static void synchronizeAccuracy(acc& a, acc& b) {
        if (a.entry.accuracy == b.entry.accuracy) return;

        if (a.entry.accuracy < b.entry.accuracy) {
            // 为a添加尾随零
            maxtype diff = b.entry.accuracy - a.entry.accuracy;
            for (maxtype i = 0; i < diff; i++) {
                a.entry.data.push_back(0);
            }
            a.entry.accuracy = b.entry.accuracy;
        }
        else {
            // 为b添加尾随零
            maxtype diff = a.entry.accuracy - b.entry.accuracy;
            for (maxtype i = 0; i < diff; i++) {
                b.entry.data.push_back(0);
            }
            b.entry.accuracy = a.entry.accuracy;
        }
    }

    // 工具函数：比较绝对值大小
    static int compareAbsolute(const acc& a, const acc& b) {
        acc aCopy = a;
        acc bCopy = b;
        synchronizeAccuracy(aCopy, bCopy);

        // 比较整数部分长度
        maxtype aIntDigits = aCopy.entry.data.size() - aCopy.entry.accuracy;
        maxtype bIntDigits = bCopy.entry.data.size() - bCopy.entry.accuracy;

        if (aIntDigits != bIntDigits) {
            return (aIntDigits > bIntDigits) ? 1 : -1;
        }

        // 逐位比较
        size_t minSize = std::min(aCopy.entry.data.size(), bCopy.entry.data.size());
        for (size_t i = 0; i < minSize; i++) {
            if (aCopy.entry.data[i] != bCopy.entry.data[i]) {
                return (aCopy.entry.data[i] > bCopy.entry.data[i]) ? 1 : -1;
            }
        }

        // 如果长度不同但前面都相等，长度长的更大
        if (aCopy.entry.data.size() != bCopy.entry.data.size()) {
            return (aCopy.entry.data.size() > bCopy.entry.data.size()) ? 1 : -1;
        }

        return 0;  // 相等
    }

    // 长除法核心算法
    static acc longDivision(const acc& dividend, const acc& divisor, maxtype precision) {
        acc result;
        result.entry.accuracy = precision;

        std::deque<int8_t> quotientDigits;
        acc remainder = dividend;

        // 已经处理了多少位（包括整数和小数）
        maxtype processedDigits = 0;
        maxtype integerDigits = 0;

        // 首先处理整数部分
        while (remainder >= divisor) {
            int8_t digit = 0;
            while (remainder >= divisor) {
                remainder = remainder - divisor;
                digit++;
            }
            quotientDigits.push_back(digit);
            processedDigits++;
            integerDigits++;
        }

        // 如果没有整数部分，添加0
        if (integerDigits == 0) {
            quotientDigits.push_back(0);
            processedDigits++;
        }

        // 添加小数点
        if (precision > 0) {
            // 继续处理小数部分
            for (maxtype i = 0; i < precision; i++) {
                // "借"一位
                remainder = remainder * acc(10);

                int8_t digit = 0;
                while (remainder >= divisor) {
                    remainder = remainder - divisor;
                    digit++;
                }

                quotientDigits.push_back(digit);
                processedDigits++;
            }
        }

        // 构建结果
        result.entry.data = quotientDigits;

        // 移除前导零（但要保留小数点前的一个零）
        result.removeLeadingZeros();

        return result;
    }

    // 判断是否为0
    bool isZero() const {
        if (entry.data.empty()) return true;
        if (entry.data.size() == 1 && entry.data[0] == 0) return true;

        // 检查所有位是否都是0
        for (int8_t digit : entry.data) {
            if (digit != 0) return false;
        }
        return true;
    }

};

// 幂运算函数
acc pow(const acc& base, int exponent) {
    if (exponent == 0) return 1;
    if (exponent < 0) {
        // 简单处理：返回1除以幂
        return 1.0 / pow((long double)base, -exponent);
    }

    acc result = base;
    for (int i = 1; i < exponent; i++) {
        result = result * base;
    }
    return result;
}


// 测试函数
int main() {
    acc g = .6;
    acc val(3.0), div(2.0);
    //TODO issue:imcompable type if not long double,etc.
    std::cout << val / div << std::endl;

    std::cout << "=== 测试大整数支持 ===" << std::endl;

    // 测试大整数
    acc bigInt1("18446744073709551618");
    std::cout << "bigInt1 = " << bigInt1 << " (精度: " << bigInt1.getAccuracy() << ")" << std::endl;

    // 测试赋值运算符
    acc bigInt2;
    bigInt2 = "18446744073709551619.71";
    std::cout << "bigInt2 = " << bigInt2 << " (精度: " << bigInt2.getAccuracy() << ")" << std::endl;

    // 测试浮点数精度
    std::cout << "\n=== 测试浮点数精度 ===" << std::endl;
    acc floatNum = 456.789L;
    std::cout << "floatNum = 456.789 -> " << floatNum << " (精度: " << floatNum.getAccuracy() << ")" << std::endl;

    acc preciseFloat = 123.456789012345678L;
    std::cout << "preciseFloat = 123.456789012345678 -> " << preciseFloat << std::endl;

    // 测试运算
    std::cout << "\n=== 测试运算 ===" << std::endl;
    acc a = 123;
    acc b = 456.789L;
    std::cout << "a = " << a << std::endl;
    std::cout << "b = " << b << std::endl;
    std::cout << "a + b = " << a + b << std::endl;
    std::cout << "b - a = " << b - a << std::endl;
    std::cout << "a * b = " << a * b << std::endl;

    // 测试复合赋值
    std::cout << "\n=== 测试复合赋值 ===" << std::endl;
    acc c = 100;
    c += 50;
    std::cout << "c += 50 -> " << c << std::endl;
    c *= 2;
    std::cout << "c *= 2 -> " << c << std::endl;

    // 测试递增递减
    std::cout << "\n=== 测试递增递减 ===" << std::endl;
    acc d = 10;
    std::cout << "d = " << d << std::endl;
    std::cout << "++d = " << ++d << std::endl;
    std::cout << "d++ = " << d++ << std::endl;
    std::cout << "d = " << d << std::endl;

    std::cout << "\n所有测试完成！" << std::endl;
    return 0;
}
