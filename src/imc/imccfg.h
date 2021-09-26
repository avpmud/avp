/* Codebase macros - Change as you might need.
 * Yes, Rogel, you can gloat all you want. You win, this is cleaner, though not by a whole lot.
 */

#ifndef __IMC2CFG_H__
#define __IMC2CFG_H__

#define CH_IMCDATA(ch)           (&(ch)->GetPlayer()->m_IMC2)
#define CH_IMCLEVEL(ch)          ((ch)->GetLevel())
#define CH_IMCNAME(ch)           ((ch)->GetName())
#define CH_IMCSEX(ch)            ((ch)->GetSex())
#define CH_IMCTITLE(ch)          ((ch)->GetTitle())

#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

#endif

