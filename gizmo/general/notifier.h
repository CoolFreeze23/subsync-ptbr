#ifndef __GENERAL_NOTIFIER_H__
#define __GENERAL_NOTIFIER_H__

#include <functional>
#include <vector>
#include <cstddef>


template <typename... T>
class Notifier
{
public:
	typedef std::function<void (T...)> Listener;
	typedef std::size_t ListenerId;

	ListenerId addListener(Listener listener)
	{
		ListenerId id = m_nextId++;
		m_listeners.push_back({id, std::move(listener)});
		return id;
	}

	bool removeListener(Listener listener=nullptr)
	{
		if (listener == nullptr)
		{
			const bool hasListeners = !m_listeners.empty();
			m_listeners.clear();
			return hasListeners;
		}

		if (!m_listeners.empty())
		{
			m_listeners.pop_back();
			return true;
		}

		return false;
	}

	bool removeListenerById(ListenerId id)
	{
		for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it)
		{
			if (it->id == id)
			{
				m_listeners.erase(it);
				return true;
			}
		}
		return false;
	}

	void notify(T... args)
	{
		for (auto &entry : m_listeners)
			entry.fn(args...);
	}

private:
	struct Entry
	{
		ListenerId id;
		Listener fn;
	};

	std::vector<Entry> m_listeners;
	ListenerId m_nextId = 0;
};

#endif
