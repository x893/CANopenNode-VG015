#include "net.h"
#include "K1921VG015.h"

static const char *s_json_header =
    "Content-Type: application/json\r\n"
    "Cache-Control: no-cache\r\n"
	"Connection: keep-alive\r\n"
	"Keep-Alive: timeout=15\r\n";

#define CAN_LOG_MAX_RECORDS	100

#if 0
static size_t print_index(void (*out)(char, void *), void *ptr, va_list *ap)
{
	int *pIndex = va_arg(*ap, int *);
	return mg_xprintf(out, ptr,"%m:%u", MG_ESC(("index")), *pIndex);
}

static size_t print_events(void (*out)(char, void *), void *ptr, va_list *ap)
{
	size_t len = 0;
	int *pIndex = va_arg(*ap, int *);
	int index = *pIndex;
	int count = 0;

	fdcan_log_item_t *can_log;

	while ((can_log = fdcan_get_log(index)) != NULL &&
			count++ < CAN_LOG_MAX_RECORDS)
	{
		len += mg_xprintf(out, ptr,
						"[%u,%u,%d,%u,[%u,%u,%u,%u,%u,%u,%u,%u]],\n",
						can_log->timestamp,
						can_log->can_id,
						can_log->dlc,
						can_log->status,
						can_log->data[0],
						can_log->data[1],
						can_log->data[2],
						can_log->data[3],
						can_log->data[4],
						can_log->data[5],
						can_log->data[6],
						can_log->data[7]
		);
		index++;
	}
	*pIndex = index;
	return len;
}
#endif

static void handle_events_get(struct mg_connection *c, struct mg_http_message *hm)
{
#if 0
	char s_value[24];
	int index = 0;
	
	s_value[0] = '\0';
	mg_http_get_var(&hm->query, "index", s_value, sizeof(s_value));
	if (!mg_str_to_num(mg_str(s_value), 10, &index, sizeof(uint8_t)))
		index = 0;

	handle_set_filtered(hm);

	mg_http_reply(c, 200, s_json_header,
		"[%M{%m:%u,%M}]\n",
		print_events, &index,
		MG_ESC(("status")), fdcan_status(),
		print_index, &index
	);
#endif
}

static int handle_send(struct mg_connection *c, struct mg_http_message *hm)
{
#if 0
	uint32_t canid;
	uint32_t dlc;
	uint8_t data[8];

	char s_value[32];

	s_value[0] = '\0';
	mg_http_get_var(&hm->query, "canid", s_value, sizeof(s_value));
	if (!mg_str_to_num(mg_str(s_value), 16, &canid, sizeof(uint32_t)))
		return 0;

	s_value[0] = '\0';
	mg_http_get_var(&hm->query, "dlc", s_value, sizeof(s_value));
	if (!mg_str_to_num(mg_str(s_value), 16, &dlc, sizeof(uint32_t)))
		return 0;
	if (dlc > 8)
		return 0;

	s_value[0] = '\0';
	mg_http_get_var(&hm->query, "data", s_value, sizeof(s_value));
	
	struct mg_str entry;
	struct mg_str s = mg_str(s_value);
	int idx = 0;

	while (idx < sizeof(data) && mg_span(s, &entry, &s, ':'))
	{
		if (!mg_str_to_num(entry, 16, &data[idx++], sizeof(uint8_t)))
			return 0;
	}

	fdcan_log_item_t message;
	message.can_id = canid;
	message.dlc = dlc;
	if (dlc != 0 && dlc <= 8)
		memcpy(message.data, data, dlc);
	return fdcan_tx_put(&message);
#endif
	return 0;
}

const struct mg_http_serve_opts http_opts = {
	.root_dir = "/web_root",
	.fs = &mg_fs_packed,
};

// HTTP request handler function
static void http_cb(struct mg_connection *c, int ev, void *ev_data)
{
	switch(ev)
	{
	case MG_EV_ACCEPT:
		MG_INFO(("HTTP CONNECT: %M", mg_print_ip4, c->rem.ip));
		break;
	case MG_EV_HTTP_MSG:
	{
		struct mg_http_message *hm = (struct mg_http_message *) ev_data;

		if (mg_match(hm->uri, mg_str("/api/events"), NULL))
		{
			handle_events_get(c, hm);
		}
		else if (mg_match(hm->uri, mg_str("/api/send"), NULL))
		{
			if (handle_send(c, hm))
				mg_http_reply(c, 200, s_json_header, "true\n");
			else
				mg_http_reply(c, 200, s_json_header, "false\n");
		}
		else
		{
			mg_http_serve_dir(c, ev_data, &http_opts);
		}
		break;
	}
	case MG_EV_CLOSE:
		c->data[0] = '\0';
		MG_INFO(("HTTP DISCONN: %M", mg_print_ip4, c->rem.ip));
		break;
	case MG_EV_ERROR:
		MG_ERROR(("HTTP ERROR:%p %s", c->fd, (char *)ev_data));
		break;
	}
}

static inline void spi_begin(void *spi)
{
	(void)spi;
	GPIOB->DATAOUTCLR = GPIO_DATAOUTSET_PIN1_Msk;
}

static inline void spi_end(void *spi)
{
	(void)spi;
	GPIOB->DATAOUTSET = GPIO_DATAOUTSET_PIN1_Msk;
}

static inline uint8_t spi_txn(void *spi, uint8_t write_byte)
{
	(void)spi;
	while ((SPI0->SR & SPI_SR_TNF_Msk) == 0)
		;
	SPI0->DR = write_byte;
	while ((SPI0->SR & SPI_SR_RNE_Msk) == 0)
		;
	uint8_t rx = SPI0->DR;
	while ((SPI0->SR & SPI_SR_BSY_Msk) != 0)
		;
	return rx;
}

bool mg_random(void *buf, size_t len)
{
	uint32_t r;
	for (size_t n = 0; n < len; n += sizeof(uint32_t))
	{
		r = rand();
		memcpy((char *) buf + n, &r, n + sizeof(r) > len ? len - n : sizeof(r));
	}
	return true;
}

// Construct MAC address from the MCU unique ID. It is defined in the
// PMUSYS->UID[0], PMUSYS->UID[1],PMUSYS->UID[2],PMUSYS->UID[3]
#define MGUID	PMUSYS->UID

static struct mg_mgr g_mgr;
static struct mg_tcpip_if mif;
static struct mg_tcpip_spi spi = {
	.spi = NULL,
	.begin = (void (*)(void *)) spi_begin,
	.end   = (void (*)(void *)) spi_end,
	.txn   = (uint8_t(*)(void *, uint8_t)) spi_txn,
};

void mongoose_init(void)
{
	MG_SET_MAC_ADDRESS(mif.mac);
	// Comment for DHCP
	mif.ip		= MG_TCPIP_IP;
	mif.mask	= MG_TCPIP_MASK;
	mif.gw		= MG_TCPIP_GW;
	mif.driver = &mg_tcpip_driver_w5500;
	mif.driver_data = &spi;

	void spi0_init(void);
	spi0_init();

	mg_log_set(MG_LL_DEBUG);
	mg_mgr_init(&g_mgr);

	srand(1963);
	mg_tcpip_init(&g_mgr, &mif);

	/*
	MG_INFO(("MAC: %M. Waiting for IP...", mg_print_mac, mif.mac));
	while (mif.state != MG_TCPIP_STATE_READY)
	{
		mg_mgr_poll(&g_mgr, 0);
	}
	 */
#ifdef HTTP_URL
	MG_INFO(("Starting HTTP listener " HTTP_URL));
	mg_http_listen(&g_mgr, HTTP_URL, http_cb, NULL);
#endif

}

void mongoose_poll(void)
{
	mg_mgr_poll(&g_mgr, 0);
}

extern volatile uint64_t MG_Ticks;
uint64_t mg_millis(void)
{
	uint64_t ticks = MG_Ticks;
	if (ticks == MG_Ticks)
		return ticks;	// second read, same value => OK
	return MG_Ticks;	// changed while reading, read again
}
