// Remote Desktop System - remote controlling of multiple PC's
// Copyright (C) 2000-2009 GravyLabs LLC

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#pragma once

#include <wincrypt.h>

class CHash
{
public:
   CHash() : m_hCryptProv(NULL), m_hHash(NULL), m_hKey(NULL)
   {
      // Get handle to the default provider. 
      if (!CryptAcquireContext(&m_hCryptProv,NULL,MS_ENHANCED_PROV,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))
      {
         if (!CryptAcquireContext(&m_hCryptProv,NULL,MS_ENHANCED_PROV,PROV_RSA_FULL,CRYPT_NEWKEYSET|CRYPT_VERIFYCONTEXT))
         {
            // If creating a new key container and an error code indicates it existed then some user information changed
            // delete the key container and re-create it
            if (!CryptAcquireContext(&m_hCryptProv,NULL,MS_ENHANCED_PROV,PROV_RSA_FULL,CRYPT_DELETEKEYSET|CRYPT_VERIFYCONTEXT))
               return;
            if (!CryptAcquireContext(&m_hCryptProv,NULL,MS_ENHANCED_PROV,PROV_RSA_FULL,CRYPT_NEWKEYSET|CRYPT_VERIFYCONTEXT))
               return;
         }
      }
   }

   ~CHash()
   {
      // Destroy the Hash
      if (m_hHash)
      {
         CryptDestroyHash(m_hHash); 
         m_hHash = NULL;
      }

      // Destroy the cryptographic session
      if (m_hKey) 
      {
         CryptDestroyKey(m_hKey);
         m_hKey = NULL;
      }
   
      // Release the cryptographic provider
      if (m_hCryptProv)
      {
         CryptReleaseContext(m_hCryptProv,0);
         m_hCryptProv = NULL;
      }
   }

   bool HashData(BYTE * pData,DWORD dwLen)
   {
      if (!m_hHash)
      {
         // Test for a good initialization
         if (!m_hCryptProv)
            return false;

         // Create the cryptographic hash
         if (!CryptCreateHash(m_hCryptProv,CALG_MD5,0,0,&m_hHash))
            return false;
      }

      // Add the data to the hash
      return CryptHashData(m_hHash,pData,dwLen,0) ? true : false;
   }

   // Get the hash 
   bool GetHash(CString & csHash)
   {
      bool bRet = false;
      if (m_hHash)
      {
         DWORD dwHashSize = 0;
         if (CryptGetHashParam(m_hHash,HP_HASHSIZE,NULL,&dwHashSize,0))
         {
            DWORD dwHashLen = 0;
            if (CryptGetHashParam(m_hHash,HP_HASHVAL,NULL,&dwHashLen,0))
            {
               std::auto_ptr<BYTE> Hash(new BYTE[dwHashLen]);
               BYTE * pbHash = Hash.get();;
               if (CryptGetHashParam(m_hHash,HP_HASHVAL,pbHash,&dwHashLen,0))
               {
                  // Create the string hash value
                  for(DWORD i = 0;i < dwHashLen;i++) // P#1089 DWORD was int
                  {
                     char sz[3] = {'\0'};
                     sprintf_s(sz,"%2.2x",pbHash[i]);
                     csHash += sz;
                  }

                  // Destroy the Hash since no more data can be added to it
                  CryptDestroyHash(m_hHash); 
                  m_hHash = NULL;
                  bRet = true;
               }
            }
         }
      }
      return bRet;
   }

protected:
   bool m_bError;
   HCRYPTPROV m_hCryptProv;
   HCRYPTHASH m_hHash;
   HCRYPTKEY m_hKey;
};