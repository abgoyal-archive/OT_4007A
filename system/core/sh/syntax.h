
/*	$NetBSD: syntax.h,v 1.2 2004/01/17 17:38:12 dsl Exp $	*/


#include <sys/cdefs.h>
#include <ctype.h>

/* Syntax classes */
#define CWORD 0			/* character is nothing special */
#define CNL 1			/* newline character */
#define CBACK 2			/* a backslash character */
#define CSQUOTE 3		/* single quote */
#define CDQUOTE 4		/* double quote */
#define CBQUOTE 5		/* backwards single quote */
#define CVAR 6			/* a dollar sign */
#define CENDVAR 7		/* a '}' character */
#define CLP 8			/* a left paren in arithmetic */
#define CRP 9			/* a right paren in arithmetic */
#define CEOF 10			/* end of file */
#define CCTL 11			/* like CWORD, except it must be escaped */
#define CSPCL 12		/* these terminate a word */

/* Syntax classes for is_ functions */
#define ISDIGIT 01		/* a digit */
#define ISUPPER 02		/* an upper case letter */
#define ISLOWER 04		/* a lower case letter */
#define ISUNDER 010		/* an underscore */
#define ISSPECL 020		/* the name of a special parameter */

#define PEOF (CHAR_MIN - 1)
#define SYNBASE (-PEOF)
/* XXX UPEOF is CHAR_MAX, so is a valid 'char' value... */
#define UPEOF ((char)PEOF)


#define BASESYNTAX (basesyntax + SYNBASE)
#define DQSYNTAX (dqsyntax + SYNBASE)
#define SQSYNTAX (sqsyntax + SYNBASE)
#define ARISYNTAX (arisyntax + SYNBASE)

/* These defines assume that the digits are contiguous */
#define is_digit(c)	((unsigned)((c) - '0') <= 9)
#define is_alpha(c)	(((char)(c)) != UPEOF && ((c) < CTL_FIRST || (c) > CTL_LAST) && isalpha((unsigned char)(c)))
#define is_name(c)	(((char)(c)) != UPEOF && ((c) < CTL_FIRST || (c) > CTL_LAST) && ((c) == '_' || isalpha((unsigned char)(c))))
#define is_in_name(c)	(((char)(c)) != UPEOF && ((c) < CTL_FIRST || (c) > CTL_LAST) && ((c) == '_' || isalnum((unsigned char)(c))))
#define is_special(c)	((is_type+SYNBASE)[c] & (ISSPECL|ISDIGIT))
#define digit_val(c)	((c) - '0')

extern const char basesyntax[];
extern const char dqsyntax[];
extern const char sqsyntax[];
extern const char arisyntax[];
extern const char is_type[];
