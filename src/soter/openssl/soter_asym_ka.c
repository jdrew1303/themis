/**
 * @file
 *
 * (c) CossackLabs
 */

#include <common/error.h>
#include "soter_openssl.h"
#include <soter/soter_ec_key.h>
#include <openssl/ec.h>

static int soter_alg_to_curve_nid(soter_asym_ka_alg_t alg)
{
	switch (alg)
	{
	case SOTER_ASYM_KA_EC_P256:
		return NID_X9_62_prime256v1;
	default:
		return 0;
	}
}

soter_status_t soter_asym_ka_init(soter_asym_ka_t* asym_ka_ctx, soter_asym_ka_alg_t alg)
{
	EVP_PKEY *pkey;
	int nid = soter_alg_to_curve_nid(alg);

	if (0 == nid)
	{
		return HERMES_INVALID_PARAMETER;
	}

	pkey = EVP_PKEY_new();
	if (!pkey)
	{
		return HERMES_NO_MEMORY;
	}

	if (!EVP_PKEY_set_type(pkey, EVP_PKEY_EC))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}

	asym_ka_ctx->pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
	if (!(asym_ka_ctx->pkey_ctx))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}

	if (1 != EVP_PKEY_paramgen_init(asym_ka_ctx->pkey_ctx))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}

	if (1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(asym_ka_ctx->pkey_ctx, nid))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}

	if (1 != EVP_PKEY_paramgen(asym_ka_ctx->pkey_ctx, &pkey))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}

	/*if (1 != EVP_PKEY_CTX_ctrl(asym_ka_ctx->pkey_ctx, EVP_PKEY_EC, -1, EVP_PKEY_CTRL_EC_PARAMGEN_CURVE_NID, nid, NULL))
	{
		EVP_PKEY_free(pkey);
		return HERMES_FAIL;
	}*/

	return HERMES_SUCCESS;
}

soter_status_t soter_asym_ka_cleanup(soter_asym_ka_t* asym_ka_ctx)
{
	if (asym_ka_ctx->pkey_ctx)
	{
		EVP_PKEY_CTX_free(asym_ka_ctx->pkey_ctx);
	}

	return HERMES_SUCCESS;
}

soter_asym_ka_t* soter_asym_ka_create(soter_asym_ka_alg_t alg)
{
	soter_status_t status;
	soter_asym_ka_t *ctx = malloc(sizeof(soter_asym_ka_t));
	if (!ctx)
	{
		return NULL;
	}

	status = soter_asym_ka_init(ctx, alg);
	if (HERMES_SUCCESS == status)
	{
		return ctx;
	}
	else
	{
		free(ctx);
		return NULL;
	}
}

soter_status_t soter_asym_ka_destroy(soter_asym_ka_t* asym_ka_ctx)
{
	soter_status_t status;

	if (!asym_ka_ctx)
	{
		return HERMES_INVALID_PARAMETER;
	}

	status = soter_asym_ka_cleanup(asym_ka_ctx);
	if (HERMES_SUCCESS == status)
	{
		free(asym_ka_ctx);
		return HERMES_SUCCESS;
	}
	else
	{
		return status;
	}
}

soter_status_t soter_asym_ka_gen_key(soter_asym_ka_t* asym_ka_ctx)
{
	EVP_PKEY *pkey = EVP_PKEY_CTX_get0_pkey(asym_ka_ctx->pkey_ctx);
	EC_KEY *ec;

	if (!pkey)
	{
		return HERMES_INVALID_PARAMETER;
	}

	if (EVP_PKEY_EC != EVP_PKEY_id(pkey))
	{
		return HERMES_INVALID_PARAMETER;
	}

	ec = EVP_PKEY_get0(pkey);
	if (NULL == ec)
	{
		return HERMES_INVALID_PARAMETER;
	}

	if (1 == EC_KEY_generate_key(ec))
	{
		return HERMES_SUCCESS;
	}
	else
	{
		return HERMES_FAIL;
	}
}

soter_status_t soter_asym_ka_import_key(soter_asym_ka_t* asym_ka_ctx, const void* key, size_t key_length)
{
	const soter_container_hdr_t *hdr = key;
	EVP_PKEY *pkey = EVP_PKEY_CTX_get0_pkey(asym_ka_ctx->pkey_ctx);

	if (!pkey)
	{
		return HERMES_INVALID_PARAMETER;
	}

	if (EVP_PKEY_EC != EVP_PKEY_id(pkey))
	{
		return HERMES_INVALID_PARAMETER;
	}

	/* TODO: what if key_length < sizeof(soter_container_hdr_t) -> buffer overflow! */
	switch (hdr->tag[0])
	{
	case 'R':
		return soter_ec_priv_key_to_engine_specific(hdr, key_length, ((soter_engine_specific_ec_key_t **)&pkey));
	case 'U':
		return soter_ec_pub_key_to_engine_specific(hdr, key_length, ((soter_engine_specific_ec_key_t **)&pkey));
	default:
		return HERMES_INVALID_PARAMETER;
	}
}

soter_status_t soter_asym_ka_export_key(soter_asym_ka_t* asym_ka_ctx, void* key, size_t* key_length, bool isprivate)
{
	EVP_PKEY *pkey = EVP_PKEY_CTX_get0_pkey(asym_ka_ctx->pkey_ctx);

	if (!pkey)
	{
		return HERMES_INVALID_PARAMETER;
	}

	if (EVP_PKEY_EC != EVP_PKEY_id(pkey))
	{
		return HERMES_INVALID_PARAMETER;
	}

	if (isprivate)
	{
		return soter_engine_specific_to_ec_priv_key((const soter_engine_specific_ec_key_t *)pkey, (soter_container_hdr_t *)key, key_length);
	}
	else
	{
		return soter_engine_specific_to_ec_pub_key((const soter_engine_specific_ec_key_t *)pkey, (soter_container_hdr_t *)key, key_length);
	}
}

soter_status_t soter_asym_ka_derive(soter_asym_ka_t* asym_ka_ctx, const void* peer_key, size_t peer_key_length, void *shared_secret, size_t* shared_secret_length)
{
	EVP_PKEY *peer_pkey = EVP_PKEY_new();
	soter_status_t res;
	size_t out_length;

	if (NULL == peer_pkey)
	{
		return HERMES_NO_MEMORY;
	}

	res = soter_ec_pub_key_to_engine_specific((const soter_container_hdr_t *)peer_key, peer_key_length, ((soter_engine_specific_ec_key_t **)&peer_pkey));
	if (HERMES_SUCCESS != res)
	{
		EVP_PKEY_free(peer_pkey);
		return res;
	}

	if (1 != EVP_PKEY_derive_init(asym_ka_ctx->pkey_ctx))
	{
		EVP_PKEY_free(peer_pkey);
		return HERMES_FAIL;
	}

	if (1 != EVP_PKEY_derive_set_peer(asym_ka_ctx->pkey_ctx, peer_pkey))
	{
		EVP_PKEY_free(peer_pkey);
		return HERMES_FAIL;
	}

	if (1 != EVP_PKEY_derive(asym_ka_ctx->pkey_ctx, NULL, &out_length))
	{
		EVP_PKEY_free(peer_pkey);
		return HERMES_FAIL;
	}

	if (out_length > *shared_secret_length)
	{
		EVP_PKEY_free(peer_pkey);
		*shared_secret_length = out_length;
		return HERMES_BUFFER_TOO_SMALL;
	}

	if (1 != EVP_PKEY_derive(asym_ka_ctx->pkey_ctx, (unsigned char *)shared_secret, shared_secret_length))
	{
		EVP_PKEY_free(peer_pkey);
		return HERMES_FAIL;
	}

	EVP_PKEY_free(peer_pkey);
	return HERMES_SUCCESS;
}