# Firmware Naming Conventions

## General
The ADI Study Watch SDK in general follows a python coding convention, this convention is also very similar to what is current practiced by Nordic throughout the nRF5 SDK.

The full version of the convention can be found at

* [Link](https://visualgit.readthedocs.io/en/latest/pages/naming_convention.html)

Specifically for this project, here is our adaptation:

1. General
* Avoid using names that are too general or too wordy. Strike a good balance between the two.
    * Bad: data_structure, my_list, info_map, dictionary_for_the_purpose_of_storing_data_representing_word_definitions
    * Good: user_profile, menu_options, word_definitions
* <s>Don’t be a jackass and name things “O”, “l”, or “I”.</s>
* <s>When using CamelCase names, capitalize all letters of an abbreviation (e.g. HTTPServer)</s>

2. Packages (e.g. this SDK)
* Package names should be all lower case
* When multiple words are needed, an underscore should separate them
* It is usually preferable to stick to 1 word names

3. Modules
* Module names should be all lower case
* When multiple words are needed, an underscore should separate them
* It is usually preferable to stick to 1 word names

4. <s>Classes</s>
* <s>Class names should follow the UpperCaseCamelCase convention</s>
* <s>Python’s built-in classes, however are typically lowercase words</s>
* <s>Exception classes should end in “Error”</s>

5. Global (module-level) Variables
* Global variables should be all lowercase
* Words in a global variable name should be separated by an underscore

6. Instance Variables
* Instance variable names should be all lower case
* Words in an instance variable name should be separated by an underscore
* Non-public instance variables should begin with a single underscore
* If an instance name needs to be mangled, two underscores may begin its name

7. <s>Methods</s>
* <s>Method names should be all lower case</s>
* <s>Words in an method name should be separated by an underscore</s>
* <s>Non-public method should begin with a single underscore</s>
* <s>If a method name needs to be mangled, two underscores may begin its name</s>

8. <s>Method Arguments</s>
* <s>Instance methods should have their first argument named ‘self’</s>
* <s>Class methods should have their first argument named ‘cls’ </s>

9. Functions
* Function names should be all lower case
* Words in a function name should be separated by an underscore

10. Constants
* Constant names must be fully capitalized
* Words in a constant name should be separated by an underscore


## In addition

### Macro
* Macros shall be described in fully capitalized word
* When multiple words are needed, an underscore should separate them
Eg:-
```
\#define PAGE_SIZE           2048  
```

### Struct Type
Structure shall be defined as shall be ending with **_t**
```
typedef struct _m2m2_sensor_common_reg_op_16_t {
   uint16_t  address;
   uint16_t  value;
} m2m2_sensor_common_reg_op_16_t;
````

### Prefixes
The following prefixed are adapter, and each prefix will lead the name and join with an underscore:
1.	n_  - Any numerical value n_samples,
2.	r_  - Bounded range (value with min/max) r_bound
3.	e_  - Enumerated value (pre-defined values) e_state, e_event, e_result (xns: should this be fully capped? since in a sense these are constants)
4.	b_  - Boolean value b_filter_enabled
5.	c_  - Any character value (including widechar) c_key_pressed
6.	sz_  - Zero-terminated string sz_message
7.	s_  - Non-terminated string s_error_msg
8.	h_  - Handle (defined in Terminology) h_instance
9.	o_  - Any structured object o_component
10.	pf_  - Pointer to function/method  pf_callback
11.	Qualifiers
    *	Pointer to... pe_cache_mode, pp_next, pa_n_samples
    *   Array of... a_components
    *   g  - Global (file-scoped) ganLookupTable


### Tabs
Tabs shall be implemented as 4 spaces in the editor of choice.

### Indentation
* Proper indentation should be used for better readability of code. In the code block below, each block of level (for) is indented by 4 spaces from the enclosing block

```
int32_t my_func(int32_t n_my_var) {
    for (i = 0; i < LOOP_COUNT; i++) {
        code block
    }
}
```
*	Indent 'class' and 'struct' blocks
*	Indent 'switch' blocks so that the 'case X:' statements are indented in the switch block. The entire case block is indented.
*	Add extra indentation to labels so they appear 1 indent less than the current indentation, rather than being flushed to the left (the default)
*	Indent multi-line preprocessor definitions ending with a backslash
*	Indent preprocessor conditional statements to the same level as the source code

### TODO Tag
If there are some changes to be done in future or fixed, then “TODO” should be used as the string comment. This will help to search later and make this change.

### Column Width
* Lines in file should be no more than 80 characters long
    * most editors have ruler that you can set to check if it crosses the limit)
    * since this will have a large impact on formatting when code is to be used for documentation.
* Break long lines instead to fit within this character limit on a line.
* Block comments above the code block also should within 80 characters as well.

### Readability
* To improve readability, symbol declarations and comments should be aligned:

```
bool bFlag;               /* this is a flag */
ADI_DEV_HANDLE *pHandle;  /* this is a pointer */
       ADI_INT_RESULT eResult;   /* this is the result */
```
* Delete empty lines within a function
* Source file should not be too long. Default is 2000
* Reserved names should not be used for preprocessor macros
* Source files should contain the copyright notice
* At least two spaces is best between code and comments

#### Other Good Practices:
* Immediately after opening parentheses, brackets or before closing parentheses, brackets
* Immediately before the open parenthesis that starts the argument list of a function call
* Immediately before the open bracket that indicates an array indexing.
* More than one space around an assignment (or other) operator (for example, to align it with another operator on another line)
* All binary operators (e.g. ==, , !=, , <=, >=, =, +=, -= etc.) except . and ->, always have a single space placed on either side of the operator.
* Single spaces shall be used either side of the operators in the ? : ternary operation.
* Spaces shall not be used after a unary operator (+, -, !, ~, \*, & etc)
* Use a single space after a comma (,)
* A single space shall be used after keywords that are followed by parenthesis (like if and for)
* No line shall end in whitespace.
* The "pointer" qualifier (\*) shall be associated with the variable name rather than the type. There shall be no whitespace after the * character
* Each expression in a for statement shall be separated by a space
