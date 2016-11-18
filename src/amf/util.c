/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This file contains utility routines common for AvSv.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "amf/amf.h"
#include "base/osaf_extended_name.h"

int avsv_cmp_horder_sanamet(const SaNameT* sanamet1, const SaNameT *sanamet2)
{
	size_t len1 = osaf_extended_name_length(sanamet1);
	size_t len2 = osaf_extended_name_length(sanamet2);

	return (len1 > len2 ? 1 : len1 < len2 ? -1 : memcmp(osaf_extended_name_borrow(sanamet1), osaf_extended_name_borrow(sanamet2), len1));
}

/****************************************************************************
  Name          : avsv_dblist_uns32_cmp
 
  Description   : This routine compares the uint32_t keys. It is used by DLL 
                  library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_dblist_uns32_cmp(uint8_t *key1, uint8_t *key2)
{
	uint32_t val1, val2;

	val1 = *((uint32_t *)key1);
	val2 = *((uint32_t *)key2);

	return ((val1 == val2) ? 0 : ((val1 > val2) ? 1 : 2));
}

/****************************************************************************
  Name          : avsv_dblist_uns64_cmp
 
  Description   : This routine compares the uint64_t keys. It is used by DLL 
                  library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_dblist_uns64_cmp(uint8_t *key1, uint8_t *key2)
{
	uint64_t val1, val2;

	val1 = *((uint64_t *)key1);
	val2 = *((uint64_t *)key2);

	return ((val1 == val2) ? 0 : ((val1 > val2) ? 1 : 2));
}

/****************************************************************************
  Name          : avsv_dblist_saname_cmp
 
  Description   : This routine compares the SaNameT keys. It is used by DLL 
                  library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_dblist_saname_cmp(uint8_t *key1, uint8_t *key2)
{
	int i = 0;
	SaNameT name1_net, name2_net;

	name1_net = *((SaNameT *)key1);
	name2_net = *((SaNameT *)key2);

	i = avsv_cmp_horder_sanamet(&name1_net, &name2_net);

	return ((i == 0) ? 0 : ((i > 0) ? 1 : 2));
}

/****************************************************************************
  Name          : avsv_dblist_sahckey_cmp
 
  Description   : This routine compares the SaAmfHealthcheckKeyT keys. It is 
                  used by DLL library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_dblist_sahckey_cmp(uint8_t *key1, uint8_t *key2)
{
	int i = 0;
	SaAmfHealthcheckKeyT hc_key1, hc_key2;

	hc_key1 = *((SaAmfHealthcheckKeyT *)key1);
	hc_key2 = *((SaAmfHealthcheckKeyT *)key2);

	i = m_CMP_HORDER_SAHCKEY(hc_key1, hc_key2);

	return ((i == 0) ? 0 : ((i > 0) ? 1 : 2));
}

/****************************************************************************
  Name          : avsv_sa_name_is_null
 
  Description   : This routine determines if sa-name is NULL.
 
  Arguments     : name - ptr to the name
 
  Return Values : true / false
 
  Notes         : None.
******************************************************************************/
bool avsv_sa_name_is_null(SaNameT *name)
{
	SaNameT null_name;

	osaf_extended_name_clear(&null_name);

	if (!avsv_cmp_horder_sanamet(name, &null_name))
		return true;
	else
		return false;
}

/**
 * Create a DN for an association class. Escape commas in the child DN.
 * @param child_dn
 * @param parent_dn
 * @param rdn_tag
 * @param dn[out]
 */
void avsv_create_association_class_dn(const SaNameT *child_dn, const SaNameT *parent_dn,
	const char *rdn_tag, SaNameT *dn)
{
	size_t parent_dn_len = 0;
	size_t child_dn_len = 0;
	size_t rdn_tag_len = 0;
	SaConstStringT child_dn_ptr = 0;
	SaConstStringT parent_dn_ptr = 0;
	int num_of_commas_in_child_dn = 0;
	
	if (child_dn) {
		child_dn_len = osaf_extended_name_length(child_dn);
		child_dn_ptr = osaf_extended_name_borrow(child_dn);

		const char* p_tmp = child_dn_ptr;
		while(*p_tmp) {
			if(*p_tmp++ == ',') num_of_commas_in_child_dn++;
		}

	}
	if (parent_dn) {
		parent_dn_len = osaf_extended_name_length(parent_dn);
		parent_dn_ptr = osaf_extended_name_borrow(parent_dn);
	}

	if (rdn_tag) {
		rdn_tag_len = strlen(rdn_tag);
	}

	// The + 3 is for,  1. rdn_tag equal char 2. child parent separation comma char and 3. terminating null char
	size_t buf_len = child_dn_len + parent_dn_len + rdn_tag_len + num_of_commas_in_child_dn + 3;
	char *buf = (char*) calloc(1, buf_len);
	char *p = buf;
	int i;

	if (rdn_tag) {
		p += snprintf(buf, buf_len, "%s=", rdn_tag);
	}

	/* copy child DN and escape commas */
	for (i = 0; i < child_dn_len; i++) {
		if (child_dn_ptr[i] == ',')
			*p++ = 0x5c; /* backslash */

		*p++ = child_dn_ptr[i];
	}

	if (parent_dn != NULL) {
		*p++ = ',';
		strcpy(p, parent_dn_ptr);
	}

	if (dn) {
		osaf_extended_name_steal(buf, dn);
	}
}

void avsv_sanamet_init_from_association_dn(const SaNameT *haystack, SaNameT *dn,
       const char *needle, const char *parent)
{
       char *p;
       char *pp;
       int i = 0;

       osaf_extended_name_clear(dn);
       /* find what we actually are looking for */
       p = strstr(osaf_extended_name_borrow(haystack), needle);
       osafassert(p);

       /* find the parent */
       pp = strstr(osaf_extended_name_borrow(haystack), parent);
       osafassert(pp);

       /* position at parent separtor */
       pp--;

       /* copy the value upto parent but skip escape chars */
       int size = 0;
       char* p1 = p;
       char* pp1 = pp;
       while (p != pp) {
               if (*p != '\\')
                       size++;
               p++;
       }
       char *buf = (char*) calloc(1, size+1);
       while (p1 != pp1) {
               if (*p1 != '\\')
            	   buf[i++] = *p1;
               p1++;
       }
       buf[i] = '\0';

       if (dn)
    	   osaf_extended_name_steal(buf, dn);
}

/**
 * Convert a SAF AMF Component category bit field to a AVSV Comp type value.
 * @param saf_comp_category
 * 
 * @return AVSV_COMP_TYPE_VAL
 */
AVSV_COMP_TYPE_VAL avsv_amfcompcategory_to_avsvcomptype(SaAmfCompCategoryT saf_comp_category)
{
	AVSV_COMP_TYPE_VAL avsv_comp_type = AVSV_COMP_TYPE_INVALID;

	if (saf_comp_category & SA_AMF_COMP_SA_AWARE) {
		if ((saf_comp_category & ~(SA_AMF_COMP_SA_AWARE | SA_AMF_COMP_LOCAL)) == 0)
			return AVSV_COMP_TYPE_SA_AWARE;
		else
			return AVSV_COMP_TYPE_INVALID;
	} 

	if (saf_comp_category & SA_AMF_COMP_PROXY) {
		if ((saf_comp_category & ~(SA_AMF_COMP_PROXY | SA_AMF_COMP_LOCAL | SA_AMF_COMP_SA_AWARE)) == 0)
			return AVSV_COMP_TYPE_SA_AWARE;
		else
			return AVSV_COMP_TYPE_INVALID;
	} 

	if ((saf_comp_category & SA_AMF_COMP_PROXIED) && (saf_comp_category &  SA_AMF_COMP_LOCAL)) {
		if ((saf_comp_category & ~(SA_AMF_COMP_PROXIED | SA_AMF_COMP_LOCAL)) == 0)
			return AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE;
		else
			return AVSV_COMP_TYPE_INVALID;
	} 

	if ((saf_comp_category & SA_AMF_COMP_PROXIED_NPI) && (saf_comp_category & SA_AMF_COMP_LOCAL)) {
		if ((saf_comp_category & ~(SA_AMF_COMP_PROXIED_NPI | SA_AMF_COMP_LOCAL)) == 0)
			return AVSV_COMP_TYPE_PROXIED_LOCAL_NON_PRE_INSTANTIABLE;
		else
			return AVSV_COMP_TYPE_INVALID;
	} 

	if (saf_comp_category & SA_AMF_COMP_LOCAL) {
		if ((saf_comp_category & ~SA_AMF_COMP_LOCAL) == 0)
			return AVSV_COMP_TYPE_NON_SAF;
		else
			return AVSV_COMP_TYPE_INVALID;
	} 

	if ((saf_comp_category == 0) || (saf_comp_category == SA_AMF_COMP_PROXIED))
		return AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE;

	if (saf_comp_category & SA_AMF_COMP_PROXIED_NPI) {
		if ((saf_comp_category & ~SA_AMF_COMP_PROXIED_NPI) == 0)
			return AVSV_COMP_TYPE_NON_SAF;
		else
			return AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE;
	} 

	/* Container type not yet supported, will return AVSV_COMP_TYPE_INVALID */

	return avsv_comp_type;
}

/****************************************************************************
  Name          : avsv_dblist_sastring_cmp
 
  Description   : This routine compares the SaStringT keys. It is used by DLL 
                  library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avsv_dblist_sastring_cmp(uint8_t *key1, uint8_t *key2)
{
	int i = 0;
	SaStringT str1, str2;

	str1 = (SaStringT)key1;
	str2 = (SaStringT)key2;

	i = strcmp(str1, str2);

	return ((i ==0) ? 0: ((i>0) ? 1 : 2));
}
